#include "Game.h"

void GameState::Setup(Difficulty diff, uint8_t player_count, std::mt19937* externalRng)
{
	uint32_t seed = (*externalRng)();
	this->rng.seed(seed);

	currentState = State::InProgress;

	gameFlags.Init();
	cityState.Init();
	players.Init(player_count);
	decks.infection_deck.Init(&rng);
	decks.player_deck.Init(&rng);

	InfectCitiesSetup();
	DealPlayerCards();
	InsertEpidemicCards(diff);
	SetRoles();
}

void GameState::InfectCitiesSetup()
{
	uint8_t cityId;

	for (int i = 0; i < 3; i++) {
		cityId = decks.infection_deck.DrawAndDiscard();
		cityState.AddDiseases(cityId, CardRegistry::GetColor(cityId), 3);
	}

	for (int i = 0; i < 3; i++) {
		cityId = decks.infection_deck.DrawAndDiscard();
		cityState.AddDiseases(cityId, CardRegistry::GetColor(cityId), 2);
	}

	for (int i = 0; i < 3; i++) {
		cityId = decks.infection_deck.DrawAndDiscard();
		cityState.AddDiseases(cityId, CardRegistry::GetColor(cityId), 1);
	}
}

void GameState::DealPlayerCards()
{
	uint8_t cardsToDeal = 6 - players.count;

	for (int playerId = 0; playerId < players.count; playerId++) {
		for (int j = 0; j < cardsToDeal; j++) {
			players.AddCard(playerId, decks.player_deck.DrawAndRemove());
		}
	}
}

void GameState::InsertEpidemicCards(Difficulty diff)
{
	uint8_t numEpidemics = MIN_EPIDEMIC_CARD + (uint8_t)diff;

	int totalPlayerCards = decks.player_deck.Count();
	int baseSize = totalPlayerCards / numEpidemics;
	int remainder = totalPlayerCards % numEpidemics;

	// We track the "current bottom" of the pile we are working on.
	// Because InsertAt shifts cards UP, it's actually easier to work 
	// from the TOP of the deck down to the bottom to keep indices stable.
	int currentTopLimit = totalPlayerCards;

	for (int i = 0; i < numEpidemics; ++i) {
		// Piles are processed top-to-bottom here.
		// The first 'remainder' piles are the larger ones (placed on top).
		int currentPileSize = baseSize + (i < remainder ? 1 : 0);

		// Calculate the range for this pile
		int pileStart = currentTopLimit - currentPileSize;
		int pileEnd = currentTopLimit; // Inclusive of the new card slot

		// Pick a random spot within this specific pile's bounds
		std::uniform_int_distribution<int> dist(pileStart, pileEnd);
		int randomIndex = dist(rng);

		decks.player_deck.InsertAt(randomIndex, CardRegistry::GetEpidemicCardID());

		// Move the limit down to the start of the next pile
		// (Note: we don't subtract 1 for the epidemic because InsertAt 
		// already pushed the 'pileStart' cards further down the array)
		currentTopLimit = pileStart;
	}
}

void GameState::SetRoles()
{
	uint8_t available_roles[NUMBER_OF_ROLE_CARDS] = { 0, 1, 2, 3, 4, 5, 6 };

	for (uint8_t i = 0; i < players.count; i++) {

		// Pick a random index from the remaining unassigned pool (i to 6)
		std::uniform_int_distribution<int> dist(i, 6);
		int rand_idx = dist(rng);

		// Swap the randomly chosen role into the current 'i' slot
		uint8_t temp = available_roles[i];
		available_roles[i] = available_roles[rand_idx];
		available_roles[rand_idx] = temp;

		// 3. Assign the definitively unique role to the player
		players.roles[i] = available_roles[i];
		if (available_roles[i] == Role::Quarantine) {
			gameFlags.SetQuarantineFlag(true);
			gameFlags.SetQuarantineSpecialistID(i);
		}
	}
}

void GameState::DoForecastSmart(uint8_t executing_player_id)
{
	// 1. Get the current top 6 cards (without modifying the deck)
	uint8_t top_six[6];
	int num_cards = decks.infection_deck.PeekForecastCards(top_six);

	struct CardData {
		int original_index;
		uint8_t city_id;
		int danger_score;
	};

	CardData cards[6];

	// 2. Evaluate the danger of each card
	for (int i = 0; i < 6; i++) {
		cards[i].original_index = i;
		cards[i].city_id = top_six[i];

		// Calculate Danger
		if (cityState.HasHotspot(cards[i].city_id)) {
			cards[i].danger_score = 100;
		}
		else {
			// Otherwise, danger is based on how many cubes are already there
			int total_cubes = 0;
			total_cubes += cityState.GetCubeCount(cards[i].city_id, ColorType::BLACK);
			total_cubes += cityState.GetCubeCount(cards[i].city_id, ColorType::BLUE);
			total_cubes += cityState.GetCubeCount(cards[i].city_id, ColorType::RED);
			total_cubes += cityState.GetCubeCount(cards[i].city_id, ColorType::YELLOW);

			cards[i].danger_score = total_cubes;
		}
	}

	// 3. Sort the cards by Danger Score (Ascending)
	// Lowest danger (0 cubes) goes to index 0 (Top of deck, drawn next)
	// Highest danger (Hotspots) goes to index 5 (Bottom of the 6, drawn last)
	std::sort(cards, cards + 6, [](const CardData& a, const CardData& b) {
		return a.danger_score < b.danger_score;
		});

	// 4. Map the sorted result to the indices required by ResolveForecast
	uint8_t target_positions[6];

	for (int new_pos = 0; new_pos < 6; new_pos++) {
		int orig_idx = cards[new_pos].original_index;
		target_positions[orig_idx] = new_pos;
	}

	// 5. Apply
	DoForecast(executing_player_id,
		target_positions[0], target_positions[1], target_positions[2],
		target_positions[3], target_positions[4], target_positions[5]
	);
}

void GameState::DoSmartDiscover(uint8_t player_id, ColorType color)
{
	std::vector<uint8_t> cards = GetBestCardsForCure(player_id, color);

	if (cards.size() < 4) {
		std::cout << std::format("<4 cards bro! Check it out:\nPlayer: {} Color: {}\n", player_id, (int)color);
		players.Print();
	}

	if (cards.size() > 4) {
		DoDiscover(
			cards[0],
			cards[1],
			cards[2],
			cards[3],
			cards[4]
		);
	}
	else {
		DoDiscover(
			cards[0],
			cards[1],
			cards[2],
			cards[3]
		);
	}

	gameFlags.UseAction();
}

void GameState::GetPossibleActions(ActionList& list) const
{
	list.count = 0;

	// ===== Flags =====
	uint8_t currentPlayer = gameFlags.GetActivePlayer();
	uint8_t currentCity = players.GetLocation(currentPlayer);
	const uint8_t* neighbors = MapData::GetNeighbors(currentCity);
	bool canCharter = false;

	struct ColorCards {
		std::array<uint8_t, HAND_LIMIT> cards;
		uint8_t card_count;

		ColorCards() {
			cards = { 255, 255, 255, 255, 255, 255, 255 };
			card_count = 0;
		}

		void Add(uint8_t card) {
			cards[card_count] = card;
			card_count++;
		}

		void GenerateCureCombinations(int required_to_cure, ColorType cure_color, ActionList& list) {
			if (card_count < required_to_cure) return;

			uint32_t mask = (1 << required_to_cure) - 1;
			uint32_t limit = (1 << card_count);

			while (mask < limit) {

				uint8_t selected_cards[5] = { 255, 255, 255, 255, 255 };
				int idx = 0;

				uint32_t temp_mask = mask;
				while (temp_mask > 0) {

					int bit_idx = std::countr_zero(temp_mask);

					selected_cards[idx++] = cards[bit_idx];

					temp_mask &= (temp_mask - 1);
				}

				list.Add(CURE, cure_color,
					selected_cards[0], selected_cards[1], selected_cards[2],
					selected_cards[3], selected_cards[4]);

				uint32_t c = mask & (~mask + 1);
				uint32_t r = mask + c;
				mask = (((r ^ mask) >> 2) / c) | r;
			}
		}
	};

	struct CardCounter {
		ColorCards red_cards;
		ColorCards yellow_cards;
		ColorCards blue_cards;
		ColorCards black_cards;

		void Add(uint8_t card) {
			ColorType color = CardRegistry::GetColor(card);

			switch (color) {
			case ColorType::BLACK:
				black_cards.Add(card);
				break;
			case ColorType::BLUE:
				blue_cards.Add(card);
				break;
			case ColorType::RED:
				red_cards.Add(card);
				break;
			case ColorType::YELLOW:
				yellow_cards.Add(card);
				break;
			}
		}

		void GenerateCureCombinations(int required_to_cure, ActionList& list) {
			red_cards.GenerateCureCombinations(required_to_cure, ColorType::RED, list);
			blue_cards.GenerateCureCombinations(required_to_cure, ColorType::BLUE, list);
			black_cards.GenerateCureCombinations(required_to_cure, ColorType::BLACK, list);
			yellow_cards.GenerateCureCombinations(required_to_cure, ColorType::YELLOW, list);
		}
	};

	CardCounter cure_counter;

	// Player don't have to use all 4 actions
	// End turn action added
	//list.Add(END_TURN);

	// ===== Hand limit =====
	// If the player has more than 7 cards, he needs to get rid of some
	// Thats the only action he can do that turn (not counted as action)
	// Event card can be used instead of discarding a card
	if (players.GetHandSize(currentPlayer) > HAND_LIMIT) 
	{
		uint64_t temp_hand = players.hands[currentPlayer];

		while (temp_hand > 0) {
			uint8_t cardId = std::countr_zero(temp_hand);

			// TODO: Player could theoretically want to discard an Event Card
			// which is currently not possible. 
			if (CardRegistry::IsEvent(cardId)) {
				AddEventAction(list, cardId, currentPlayer);
			}
			else {
				list.Add(DISCARD_CARD, cardId, currentPlayer, currentPlayer);
			}

			temp_hand &= (temp_hand - 1);
		}

		return;
	}

	// ===== Station limit =====
	// Player can place max 6 research stations
	if (cityState.GetStationCount() > MAX_RESEARCH_LAB_COUNT) {

		uint64_t station_mask = cityState.GetStationMask();

		while (station_mask > 0) {
			uint8_t city_id = std::countr_zero(station_mask);

			list.Add(REMOVE_STATION, city_id, currentPlayer, currentPlayer);

			station_mask &= (station_mask - 1);
		}

		return;

	}

	// ===== Drive ====
	// Listing neighboring cities that the current player can drive to.
	for (int i = 0; i < MapData::GetNeighborCount(currentCity); i++) {
		list.Add(DRIVE, neighbors[i], currentPlayer, currentPlayer);
	}

	// ===== Hand Loop =====
	// A lot of actions depend on cards the player is holding.
	// We need to check all the players becuase a player can play an event card
	// even if it is not his turn.
	for (int player_id = 0; player_id < players.count; player_id++) {

		// Share knowledge
		// Case: None of the players are Researchers 
		// Rest of the cases solved below
		if (player_id != currentPlayer 
			&& players.AreTogether(player_id, currentPlayer)
			&& players.GetRole(player_id) != Role::Researcher
			&& players.GetRole(currentPlayer) != Role::Researcher) 
		{
			// Current player has the city card they are both standing on,
			// so he can GIVE it.
			if (players.HasCard(currentPlayer, currentCity))
				list.Add(SHARE, true, currentCity, currentPlayer, player_id);

			// Player has the city card the are both standing on,
			// so current player can TAKE it.
			else if (players.HasCard(player_id, currentCity))
				list.Add(SHARE, false, currentCity, player_id, currentPlayer);
		}

		uint64_t temp_hand = players.hands[player_id];

		while (temp_hand > 0) {
			uint8_t cardId = std::countr_zero(temp_hand);

			if (player_id != currentPlayer) {

				// Player can play an Event Card even if it's not their turn
				if (CardRegistry::IsEvent(cardId)) {
					AddEventAction(list, cardId, player_id);
				}

				// Share Knowledge
				// Case: Player is the the Researcher, so current player can TAKE any of his cards
				if (players.AreTogether(currentPlayer, player_id)
					&& players.GetRole(player_id) == Role::Researcher) 
				{
					list.Add(SHARE, false, cardId, player_id, currentPlayer);
				}

				temp_hand &= (temp_hand - 1);
				continue;
			}

			// Discover a Cure (preparation)
			cure_counter.Add(cardId);

			// Charter Flight Flag
			if (cardId == currentCity) {
				canCharter = true;
				// Build Station
				if (!cityState.HasStation(currentCity))
					list.Add(BUILD, currentCity, currentPlayer, currentPlayer);
			}

			// Direct Flight
			if (!CardRegistry::IsEvent(cardId) && cardId != currentCity)
				list.Add(DIRECT_FLIGHT, cardId, currentPlayer, currentPlayer);

			// Share Knowledge
			// Case: Current player is the Researcher, so he can GIVE any of his cards
			if (players.AreTogether(currentPlayer, player_id)
				&& players.GetRole(currentPlayer) == Role::Researcher) 
			{
				list.Add(SHARE, true, cardId, currentPlayer, player_id);
			}

			// Events
			if (CardRegistry::IsEvent(cardId)) {
				AddEventAction(list, cardId, currentPlayer);
			}

			temp_hand &= (temp_hand - 1);
		}
	}

	// ===== Cure =====
	if (cityState.HasStation(currentCity)) {
		cure_counter.GenerateCureCombinations(
			players.GetRole(currentPlayer) == Role::Scientist ? 4 : 5,
			list
		);
	}

	// ===== Charter =====
	if (canCharter) {

		for (uint8_t cityId = 0; cityId < NUMBER_OF_CITIES; cityId++) {
			// Skip flying to a city the player is already in
			if (currentCity != cityId)
				list.Add(CHARTER_FLIGHT, cityId, currentPlayer, currentPlayer);
		}
	}

	// ===== Shuttle =====
	if (cityState.HasStation(currentCity)) {
		uint64_t available_stations = cityState.GetStationMask();

		// Remove the city the player is currently in 
		available_stations &= ~(1ULL << currentCity);

		while (available_stations > 0) {
			uint8_t targetCity = std::countr_zero(available_stations);

			list.Add(SHUTTLE_FLIGHT, targetCity, currentPlayer, currentPlayer);

			available_stations &= (available_stations - 1);
		}
	}

	// ===== Treat =====
	if (cityState.HasDisease(currentCity, BLUE)) {
		list.Add(TREAT, currentCity, currentPlayer, BLUE);
	}
	if (cityState.HasDisease(currentCity, YELLOW)) {
		list.Add(TREAT, currentCity, currentPlayer, YELLOW);
	}
	if (cityState.HasDisease(currentCity, BLACK)) {
		list.Add(TREAT, currentCity, currentPlayer, BLACK);
	}
	if (cityState.HasDisease(currentCity, RED)) {
		list.Add(TREAT, currentCity, currentPlayer, RED);
	}

	// ===== Roles =====
	switch (players.GetRole(currentPlayer)) {
	case Role::Contingency:
		AddConPlannerActions(list);
		break;

	case Role::Dispatcher:
		AddDispatcherActions(list);
		break;

	case Role::Operations:
		AddOpsExpertActions(list);
		break;
	}
}

void GameState::GetFilteredActions(ActionList& list) const
{
	/*
	Actions filtered for the AI, so the search space is not huge.
	*/

	list.count = 0;

	// ===== Flags =====
	uint8_t currentPlayer = gameFlags.GetActivePlayer();
	uint8_t currentCity = players.GetLocation(currentPlayer);
	const uint8_t* neighbors = MapData::GetNeighbors(currentCity);
	bool canCharter = false;

	struct ColorCards {
		std::array<uint8_t, HAND_LIMIT> cards;
		uint8_t card_count;

		ColorCards() {
			cards = { 255, 255, 255, 255, 255, 255, 255 };
			card_count = 0;
		}

		void Add(uint8_t card) {
			cards[card_count] = card;
			card_count++;
		}

		void GenerateCureCombinations(int required_to_cure, ColorType cure_color, ActionList& list) {
			if (card_count < required_to_cure) return;

			uint32_t mask = (1 << required_to_cure) - 1;
			uint32_t limit = (1 << card_count);

			while (mask < limit) {

				uint8_t selected_cards[5] = { 255, 255, 255, 255, 255 };
				int idx = 0;

				uint32_t temp_mask = mask;
				while (temp_mask > 0) {

					int bit_idx = std::countr_zero(temp_mask);

					selected_cards[idx++] = cards[bit_idx];

					temp_mask &= (temp_mask - 1);
				}

				list.Add(CURE, cure_color,
					selected_cards[0], selected_cards[1], selected_cards[2],
					selected_cards[3], selected_cards[4]);

				uint32_t c = mask & (~mask + 1);
				uint32_t r = mask + c;
				mask = (((r ^ mask) >> 2) / c) | r;
			}
		}
	};

	struct CardCounter {
		ColorCards red_cards;
		ColorCards yellow_cards;
		ColorCards blue_cards;
		ColorCards black_cards;

		void Add(uint8_t card) {
			ColorType color = CardRegistry::GetColor(card);

			switch (color) {
			case ColorType::BLACK:
				black_cards.Add(card);
				break;
			case ColorType::BLUE:
				blue_cards.Add(card);
				break;
			case ColorType::RED:
				red_cards.Add(card);
				break;
			case ColorType::YELLOW:
				yellow_cards.Add(card);
				break;
			}
		}

		void GenerateCureCombinations(int required_to_cure, ActionList& list) {
			red_cards.GenerateCureCombinations(required_to_cure, ColorType::RED, list);
			blue_cards.GenerateCureCombinations(required_to_cure, ColorType::BLUE, list);
			black_cards.GenerateCureCombinations(required_to_cure, ColorType::BLACK, list);
			yellow_cards.GenerateCureCombinations(required_to_cure, ColorType::YELLOW, list);
		}
	};

	CardCounter cure_counter;

	// Player don't have to use all 4 actions
	// End turn action added
	// list.Add(END_TURN);

	// ===== Hand limit =====
	// If the player has more than 7 cards, he needs to get rid of some
	// Thats the only action he can do that turn (not counted as action)
	// Event card can be used instead of discarding a card
	// Filter: Getting rid of only the least frequent colored cards
	if (players.GetHandSize(currentPlayer) > HAND_LIMIT)
	{
		uint64_t temp_hand = players.hands[currentPlayer];
		ColorCount leastColor = players.GetLeastFrequentColor(currentPlayer);

		while (temp_hand > 0) {
			uint8_t cardId = std::countr_zero(temp_hand);

			if (CardRegistry::IsEvent(cardId)) {
				AddFilteredEventAction(list, cardId, currentPlayer);
			}

			else if (CardRegistry::GetColor(cardId) == leastColor.color) {
				list.Add(DISCARD_CARD, cardId, currentPlayer, currentPlayer);
			}

			temp_hand &= (temp_hand - 1);
		}

		return;
	}

	// ===== Station limit =====
	// Player can place max 6 research stations
	// Filter: Remove station that is closest to every other 
	// (We want them to be evenly spread around the map)
	if (cityState.GetStationCount() > MAX_RESEARCH_LAB_COUNT) {
		list.Add(REMOVE_STATION, GetTheWorstStation(), currentPlayer, currentPlayer);
		return;
	}

	// ===== Drive ====
	// Listing neighboring cities that the current player can drive to.
	for (int i = 0; i < MapData::GetNeighborCount(currentCity); i++) {
		list.Add(DRIVE, neighbors[i], currentPlayer, currentPlayer);
	}

	// ===== Hand Loop =====
	// A lot of actions depend on cards the player is holding.
	// We need to check all the players becuase a player can play an event card
	// even if it is not his turn.
	for (int player_id = 0; player_id < players.count; player_id++) {

		// Share knowledge
		// Case: None of the players are Researchers 
		// Rest of the cases solved below
		if (player_id != currentPlayer
			&& players.AreTogether(player_id, currentPlayer)
			&& players.GetRole(player_id) != Role::Researcher
			&& players.GetRole(currentPlayer) != Role::Researcher)
		{
			// Current player has the city card they are both standing on,
			// so he can GIVE it.
			if (players.HasCard(currentPlayer, currentCity))
				list.Add(SHARE, true, currentCity, currentPlayer, player_id);

			// Player has the city card the are both standing on,
			// so current player can TAKE it.
			else if (players.HasCard(player_id, currentCity))
				list.Add(SHARE, false, currentCity, player_id, currentPlayer);
		}

		uint64_t temp_hand = players.hands[player_id];

		while (temp_hand > 0) {
			uint8_t cardId = std::countr_zero(temp_hand);

			if (player_id != currentPlayer) {

				// Player can play an Event Card even if it's not their turn
				if (CardRegistry::IsEvent(cardId)) {
					AddFilteredEventAction(list, cardId, player_id);
				}

				// Share Knowledge
				// Case: Player is the the Researcher, so current player can TAKE any of his cards
				if (players.AreTogether(currentPlayer, player_id)
					&& players.GetRole(player_id) == Role::Researcher)
				{
					list.Add(SHARE, false, cardId, player_id, currentPlayer);
				}

				temp_hand &= (temp_hand - 1);
				continue;
			}

			// Discover a Cure (preparation)
			cure_counter.Add(cardId);

			// Charter Flight Flag
			if (cardId == currentCity) {
				canCharter = true;
				// Build Station
				// Filter: Only build if there is no station around
				if (!cityState.HasStation(currentCity) && cityState.GetDistanceToNearestStation(currentCity) > 2) {
					list.Add(BUILD, currentCity, currentPlayer, currentPlayer);
				}
			}

			// Direct Flight
			if (!CardRegistry::IsEvent(cardId) && cardId != currentCity)
				list.Add(DIRECT_FLIGHT, cardId, currentPlayer, currentPlayer);

			// Share Knowledge
			// Case: Current player is the Researcher, so he can GIVE any of his cards
			// TODO: Think of SHARE filter
			if (players.AreTogether(currentPlayer, player_id)
				&& players.GetRole(currentPlayer) == Role::Researcher)
			{
				list.Add(SHARE, true, cardId, currentPlayer, player_id);
			}

			// Events
			if (CardRegistry::IsEvent(cardId)) {
				AddFilteredEventAction(list, cardId, currentPlayer);
			}

			temp_hand &= (temp_hand - 1);
		}
	}

	// ===== Cure =====
	// TODO: Filter, choose one
	if (cityState.HasStation(currentCity)) {
		cure_counter.GenerateCureCombinations(
			players.GetRole(currentPlayer) == Role::Scientist ? 4 : 5,
			list
		);
	}

	// ===== Charter =====
	// Filter: Can charter only 
	// (a) To other player (for possible share knowledge)
	// (b) To hotspot or it's neighbor (hotspot = city with 3 same colored disease cubes)
	// (c) To a station (for possible cure discover)
	if (canCharter) {

		// Using bitmask to not allow duplicates
		uint64_t target_mask = 0;

		// To other players
		for (int player_id = 0; player_id < players.count; player_id++) {
			if (player_id == currentPlayer) continue;
			target_mask |= (1ULL << players.GetLocation(player_id));
		}

		// To Research Stations
		target_mask |= cityState.GetStationMask();

		// To Hotspots AND their neighbors
		uint64_t hotspot_mask = cityState.GetHotspotMask();
		target_mask |= hotspot_mask;

		// Loop through hotspots to add their neighbors
		while (hotspot_mask > 0) {
			uint8_t city_id = std::countr_zero(hotspot_mask);

			const uint8_t* neighbors = MapData::GetNeighbors(city_id);
			while (*neighbors != 255) {
				target_mask |= (1ULL << *neighbors); // Add the neighbor
				neighbors++;
			}
			hotspot_mask &= (hotspot_mask - 1);
		}

		// Remove the city we are currently standing in
		target_mask &= ~(1ULL << currentCity);

		// Extract the unique actions
		while (target_mask > 0) {
			uint8_t target_city = std::countr_zero(target_mask);
			list.Add(CHARTER_FLIGHT, target_city, currentPlayer, currentPlayer);
			target_mask &= (target_mask - 1);
		}
	}

	// ===== Shuttle =====
	if (cityState.HasStation(currentCity)) {
		uint64_t available_stations = cityState.GetStationMask();

		// Remove the city the player is currently in 
		available_stations &= ~(1ULL << currentCity);

		while (available_stations > 0) {
			uint8_t targetCity = std::countr_zero(available_stations);

			list.Add(SHUTTLE_FLIGHT, targetCity, currentPlayer, currentPlayer);

			available_stations &= (available_stations - 1);
		}
	}

	// ===== Treat =====
	if (cityState.HasDisease(currentCity, BLUE)) {
		list.Add(TREAT, currentCity, currentPlayer, BLUE);
	}
	if (cityState.HasDisease(currentCity, YELLOW)) {
		list.Add(TREAT, currentCity, currentPlayer, YELLOW);
	}
	if (cityState.HasDisease(currentCity, BLACK)) {
		list.Add(TREAT, currentCity, currentPlayer, BLACK);
	}
	if (cityState.HasDisease(currentCity, RED)) {
		list.Add(TREAT, currentCity, currentPlayer, RED);
	}

	// ===== Roles =====
	switch (players.GetRole(currentPlayer)) {
	case Role::Contingency:
		if (!gameFlags.IsContingencyPlannerSlotEmpty()) {
			AddFilteredEventAction(list, gameFlags.GetContingencyPlannerSlot(), currentPlayer);
		}
		break;

	case Role::Dispatcher:
		AddFilteredDispatcherActions(list);
		break;

	case Role::Operations:
		AddFilteredOpsExpertActions(list);
		break;
	}
}

void GameState::AddEventAction(ActionList& list, uint8_t event_card_id, uint8_t card_owner_id) const
{
	switch (event_card_id) {

	case EventCardID::GovGrant:
		for (int i = 0; i < NUMBER_OF_CITIES; i++) {
			if (!cityState.HasStation(i)) {
				list.Add(GOVERNMENT_GRANT, i, card_owner_id, card_owner_id);
			}
		}
		break;

	case EventCardID::Forecast:
		list.Add(FORECAST);
		break;

	case EventCardID::ResilientPopulation:
		for (uint8_t cardId : decks.infection_deck.GetDiscardPile()) {
			list.Add(RESILIENT_POPULATION, cardId, card_owner_id, card_owner_id);
		}
		break;

	case EventCardID::OneQuietNight:
		list.Add(ONE_QUIET_NIGHT, 255, card_owner_id, card_owner_id);
		break;

	case EventCardID::Airlift:
		for (int player_id = 0; player_id < players.count; player_id++) {
			for (int city_id = 0; city_id < NUMBER_OF_CITIES; city_id++) {
				if (players.GetLocation(player_id) != city_id) {
					list.Add(AIRLIFT, city_id, card_owner_id, player_id);
				}
			}
		}
		break;
	}
}

void GameState::AddConPlannerActions(ActionList& list) const
{
	uint8_t currentPlayer = gameFlags.GetActivePlayer();

	if (!gameFlags.IsContingencyPlannerSlotEmpty()) {
		AddEventAction(list, gameFlags.GetContingencyPlannerSlot(), currentPlayer);
	}
	auto discard_pile = decks.player_deck.GetDiscardPile();
	for (const auto& card : discard_pile) {
		if (CardRegistry::IsEvent(card)) {
			list.Add(PLANNER_TAKE, card, currentPlayer, currentPlayer);
		}
	}
}

void GameState::AddDispatcherActions(ActionList& list) const
{
	uint8_t currentPlayer = gameFlags.GetActivePlayer();

	// ABILITY 1: Teleport any pawn to another pawn
	for (int pawn_to_move = 0; pawn_to_move < players.count; pawn_to_move++) {
		uint8_t start_city = players.GetLocation(pawn_to_move);

		for (int target_pawn = 0; target_pawn < players.count; target_pawn++) {
			if (pawn_to_move == target_pawn) continue;

			uint8_t dest_city = players.GetLocation(target_pawn);

			// Only add if they aren't already in the same city
			if (start_city != dest_city) {
				list.Add(DISPATCHER_MOVE, dest_city, currentPlayer, pawn_to_move);
			}
		}
	}

	// ABILITY 2: Move another pawn as your own
	for (int pawn_to_move = 0; pawn_to_move < players.count; pawn_to_move++) {
		if (pawn_to_move == currentPlayer) continue; // Handled by normal movement logic

		uint8_t pawn_loc = players.GetLocation(pawn_to_move);

		// 2A. DRIVE
		const uint8_t* neighbors = MapData::GetNeighbors(pawn_loc);
		while (*neighbors != 255) {
			list.Add(DRIVE, *neighbors, currentPlayer, pawn_to_move);
			neighbors++;
		}

		// 2B. SHUTTLE FLIGHT
		if (cityState.HasStation(pawn_loc)) {
			uint64_t stations = cityState.GetStationMask() & ~(1ULL << pawn_loc);
			while (stations > 0) {
				list.Add(SHUTTLE_FLIGHT, std::countr_zero(stations), currentPlayer, pawn_to_move);
				stations &= (stations - 1);
			}
		}

		// 2C. FLIGHTS (Using the Dispatcher's Hand)
		uint64_t temp_hand = players.hands[currentPlayer];

		while (temp_hand > 0) {
			uint8_t cardId = std::countr_zero(temp_hand);

			if (!CardRegistry::IsEvent(cardId)) {
				// DIRECT FLIGHT: Dispatcher discards 'cardId' to move pawn to 'cardId'
				if (cardId != pawn_loc) 
					list.Add(DIRECT_FLIGHT, cardId, currentPlayer, pawn_to_move);

				// CHARTER FLIGHT: Dispatcher discards card matching pawn's current location
				if (cardId == pawn_loc) {
					for (uint8_t city_id = 0; city_id < NUMBER_OF_CITIES; city_id++) {
						if (city_id != pawn_loc)
							list.Add(CHARTER_FLIGHT, city_id, currentPlayer, pawn_to_move);
					}
				}
			}
			temp_hand &= (temp_hand - 1);
		}
	}
}

void GameState::AddOpsExpertActions(ActionList& list) const {
	uint8_t currentPlayer = gameFlags.GetActivePlayer();
	uint8_t currentCity = players.GetLocation(currentPlayer);

	// ABILITY 1: Free Build
	if (!cityState.HasStation(currentCity)) {
		list.Add(EXPERT_BUILD, currentCity, currentPlayer, currentPlayer);
	}

	// ABILITY 2: Special Flight (Once per turn)
	if (cityState.HasStation(currentCity) &&
		players.GetHandSize(currentPlayer) > 0 &&
		!gameFlags.HasOpsExpertUsedFlight())
	{
		uint64_t temp_hand = players.hands[currentPlayer];

		while (temp_hand > 0) {
			uint8_t discard_card_id = std::countr_zero(temp_hand);
			if (!CardRegistry::IsEvent(discard_card_id)) {
				for (uint8_t city_id = 0; city_id < NUMBER_OF_CITIES; city_id++) {
					list.Add(EXPERT_MOVE, city_id, discard_card_id, currentPlayer, currentPlayer);
				}
			}
			temp_hand &= (temp_hand - 1);
		}
	}
}

void GameState::AddFilteredOpsExpertActions(ActionList& list) const
{
	uint8_t currentPlayer = gameFlags.GetActivePlayer();
	uint8_t currentCity = players.GetLocation(currentPlayer);

	// ABILITY 1: Free Build
	if (!cityState.HasStation(currentCity) && cityState.GetDistanceToNearestStation(currentCity) > 2) {
		list.Add(EXPERT_BUILD, currentCity, currentPlayer, currentPlayer);
	}

	// ABILITY 2: Special Flight (Once per turn)
	// 1. Must be at a station
	// 2. Must have at least 1 card
	// 3. Must not have used this ability yet this turn (Requires a tracking flag in your engine)
	if (cityState.HasStation(currentCity) &&
		players.GetHandSize(currentPlayer) > 0 &&
		!gameFlags.HasOpsExpertUsedFlight())
	{
		// Find the BEST card to burn (The "Worst" card strategically)
		uint8_t card_to_burn = 255;
		uint64_t temp_hand = players.hands[currentPlayer];

		// Simple Heuristic: 
		// 1. Prioritize cards of a CURED disease.
		// 2. Otherwise, pick a card matching the color the player has the LEAST of.
		ColorCount leastColor = players.GetLeastFrequentColor(currentPlayer);

		while (temp_hand > 0) {
			uint8_t cardId = std::countr_zero(temp_hand);
			if (!CardRegistry::IsEvent(cardId)) {
				ColorType c = CardRegistry::GetColor(cardId);

				if (gameFlags.IsCured(c)) {
					card_to_burn = cardId;
					break;
				}
				if (card_to_burn == 255 || c == leastColor.color) {
					card_to_burn = cardId;
				}
			}
			temp_hand &= (temp_hand - 1);
		}

		// If we only had Event cards, we shouldn't burn them for this
		if (card_to_burn == 255) return;

		// Find the USEFUL destinations
		uint64_t target_mask = cityState.GetHotspotMask();

		// Add cities with other players (for sharing knowledge)
		for (int p = 0; p < players.count; p++) {
			if (p != currentPlayer) target_mask |= (1ULL << players.GetLocation(p));
		}

		// FILTER: Remove stations! (Ops Expert can already shuttle to them without burning a card)
		target_mask &= ~cityState.GetStationMask();

		// FILTER: Remove current city
		target_mask &= ~(1ULL << currentCity);

		// Add the actions
		while (target_mask > 0) {
			uint8_t dest_city = std::countr_zero(target_mask);
			list.Add(EXPERT_MOVE, dest_city, card_to_burn, currentPlayer, currentPlayer);
			target_mask &= (target_mask - 1);
		}
	}
}

void GameState::AddFilteredEventAction(ActionList& list, uint8_t event_card_id, uint8_t card_owner_id) const
{
	switch (event_card_id) {

	// GOVERNMENT GRANT (Build free station)
	// Filter: Only build in the cities that are the absolute FURTHEST 
	// away from any existing research station.
	case EventCardID::GovGrant:
	{
		int max_dist = -1;
		uint64_t best_cities = 0;

		for (int i = 0; i < NUMBER_OF_CITIES; i++) {
			if (!cityState.HasStation(i)) {
				int dist = cityState.GetDistanceToNearestStation(i);

				if (dist > max_dist) {
					max_dist = dist;
					best_cities = (1ULL << i);
				}
				else if (dist == max_dist) {
					best_cities |= (1ULL << i);
				}
			}
		}

		while (best_cities > 0) {
			uint8_t target = std::countr_zero(best_cities);
			list.Add(GOVERNMENT_GRANT, target, card_owner_id, card_owner_id);
			best_cities &= (best_cities - 1);
		}
		break;
	}

	case EventCardID::Forecast:
		break;


	// RESILIENT POPULATION (Remove card from discard)
	// Filter: Only remove the cards/cities that currently have the MOST 
	// disease cubes on them (the highest outbreak risk).
	case EventCardID::ResilientPopulation:
	{
		int max_cubes = -1;
		uint64_t discard_mask = 0;

		for (uint8_t cardId : decks.infection_deck.GetDiscardPile()) {
			if (cardId < NUMBER_OF_CITIES) {
				int cubes = cityState.GetTotalCubeCount(cardId);

				if (cubes > max_cubes) {
					max_cubes = cubes;
					discard_mask = (1ULL << cardId);
				}
				else if (cubes == max_cubes) {
					discard_mask |= (1ULL << cardId);
				}
			}
		}

		// If there are three 3-cube cities in the discard, it will output all 3.
		while (discard_mask > 0) {
			uint8_t target = std::countr_zero(discard_mask);
			list.Add(RESILIENT_POPULATION, target, card_owner_id, card_owner_id);
			discard_mask &= (discard_mask - 1);
		}
		break;
	}

	// ONE QUIET NIGHT (Skip next Infection Phase)
	// Filter: Don't let the AI spam this on Turn 1. 
	// Only allow it if the board is actually dangerous!
	case EventCardID::OneQuietNight:
	{
		// Condition: Is there a 3-cube city (Hotspot)? 
		// OR is the Infection Rate drawing 3 or 4 cards?
		if (cityState.GetHotspotMask() > 0 || gameFlags.GetInfectionRateIndex() >= 4) {
			list.Add(ONE_QUIET_NIGHT, 255, card_owner_id, card_owner_id);
		}
		break;
	}

	// AIRLIFT (Move any player anywhere)
	// Filter: Only airlift players to highly strategic locations
	// (Hotspots, Research Stations, or to other players).
	case EventCardID::Airlift:
	{
		/*
		uint64_t useful_targets = cityState.GetHotspotMask() | cityState.GetStationMask();

		// Add cities with other players
		for (int p = 0; p < players.count; p++) {
			useful_targets |= (1ULL << players.GetLocation(p));
		}

		for (int player_to_move = 0; player_to_move < players.count; player_to_move++) {
			uint8_t current_loc = players.GetLocation(player_to_move);

			// Remove the player's current location from their valid targets
			uint64_t player_targets = useful_targets & ~(1ULL << current_loc);

			while (player_targets > 0) {
				uint8_t target_city = std::countr_zero(player_targets);

				list.Add(AIRLIFT, target_city, card_owner_id, player_to_move);

				player_targets &= (player_targets - 1);
			}
		}
		*/
		break;
	}
	}
}

void GameState::AddFilteredDispatcherActions(ActionList& list) const
{
	uint8_t currentPlayer = gameFlags.GetActivePlayer();

	// ABILITY 1: Teleport any pawn to another pawn
	for (int pawn_to_move = 0; pawn_to_move < players.count; pawn_to_move++) {
		uint8_t start_city = players.GetLocation(pawn_to_move);

		for (int target_pawn = 0; target_pawn < players.count; target_pawn++) {
			if (pawn_to_move == target_pawn) continue; // Can't teleport to yourself

			uint8_t dest_city = players.GetLocation(target_pawn);

			// Only add if they aren't already in the same city
			if (start_city != dest_city) {
				list.Add(DISPATCHER_MOVE, dest_city, currentPlayer, pawn_to_move);
			}
		}
	}

	// ABILITY 2: Move another pawn as your own
	for (int pawn_to_move = 0; pawn_to_move < players.count; pawn_to_move++) {
		if (pawn_to_move == currentPlayer) continue; // Handled by normal movement logic

		uint8_t pawn_loc = players.GetLocation(pawn_to_move);

		// 2A. DRIVE (Branches: ~4 per player. Safe to leave unfiltered)
		const uint8_t* neighbors = MapData::GetNeighbors(pawn_loc);
		while (*neighbors != 255) {
			list.Add(DRIVE, *neighbors, currentPlayer, pawn_to_move);
			neighbors++;
		}

		// 2B. SHUTTLE FLIGHT (Station to Station)
		if (cityState.HasStation(pawn_loc)) {
			uint64_t stations = cityState.GetStationMask() & ~(1ULL << pawn_loc);
			while (stations > 0) {
				list.Add(SHUTTLE_FLIGHT, std::countr_zero(stations), currentPlayer, pawn_to_move);
				stations &= (stations - 1);
			}
		}

		// 2C. FLIGHTS (Using the Dispatcher's Hand)
		// We only generate flights if the destination is USEFUL (Hotspot, Station, Player)
		uint64_t temp_hand = players.hands[currentPlayer];

		while (temp_hand > 0) {
			uint8_t cardId = std::countr_zero(temp_hand);

			if (!CardRegistry::IsEvent(cardId)) {

				// DIRECT FLIGHT: Dispatcher discards 'cardId' to move pawn to 'cardId'
				if (cardId != pawn_loc) {
					bool isUseful = cityState.HasStation(cardId) ||
						(cityState.GetTotalCubeCount(cardId) >= 3) ||
						players.IsAnyPlayerAt(cardId);

					if (isUseful) {
						list.Add(DIRECT_FLIGHT, cardId, currentPlayer, pawn_to_move);
					}
				}

				// CHARTER FLIGHT: Dispatcher discards card matching pawn's current location
				if (cardId == pawn_loc) {
					uint64_t target_mask = cityState.GetStationMask() | cityState.GetHotspotMask();

					// Add cities with other players
					for (int p = 0; p < players.count; p++) target_mask |= (1ULL << players.GetLocation(p));

					target_mask &= ~(1ULL << pawn_loc); // Remove current location

					while (target_mask > 0) {
						list.Add(CHARTER_FLIGHT, std::countr_zero(target_mask), currentPlayer, pawn_to_move);
						target_mask &= (target_mask - 1);
					}
				}
			}
			temp_hand &= (temp_hand - 1);
		}
	}
}

void GameState::GetPolicyTurns(TurnList& list) const
{
	AddCureTurns(list);
	AddShareTurns(list);
	AddTreatTurns(list);
	AddBuildTurns(list);
	AddWalkTurn(list); // TODO: Without this I get no moves in the MCTS, which is weird. Look into it 
}

void GameState::Execute(Turn& turn)
{
	for (const Action& action : turn) {
		Execute(action);
		if (currentState != State::InProgress) break;
	}

	HandleLimits();
}

void GameState::AddCureTurns(TurnList& list) const
{
	uint8_t currentPlayer = gameFlags.GetActivePlayer();
	uint8_t currentCity = players.GetLocation(currentPlayer);
	uint8_t actions_left = gameFlags.GetActionsRemaining();

	// Helper to safely instantiate CURE without constructor ambiguity
	auto MakeCureAction = [](ColorType color) {
		Action a;
		a.base.type = CURE;
		a.discover_cure.color_id = color;
		a.discover_cure.color_card0_id = 15;
		a.discover_cure.color_card1_id = 15;
		a.discover_cure.color_card2_id = 15;
		a.discover_cure.color_card3_id = 15;
		a.discover_cure.color_card4_id = 15;
		return a;
		};

	// 1. Cure Disease
	// Player has enough cards to cure disease, so we cure if there is 
	// a station in the city, or go to one (or towards).
	ColorType cureColor = players.GetCureColor(currentPlayer);
	if (cureColor != ColorType::NO_COLOR && !gameFlags.IsCured(cureColor)) {
		Turn cureTurn;

		// 1.0 The city already has station
		if (cityState.HasStation(currentCity)) {
			if (actions_left >= 1) {
				cureTurn.Add(MakeCureAction(cureColor));
				list.Add(cureTurn);
			}
		}

		// 1.1 Can we reach it by only driving (in THIS turn).
		uint8_t dist = cityState.GetDistanceToNearestStation(currentCity);
		if (dist > 0 && dist < actions_left) {
			cureTurn.Clear();
			uint8_t closest_station = cityState.GetNearestStation(currentCity);
			const StaticPath& drive_path = MapData::drivePaths[currentCity][closest_station];
			for (int i = 0; i < dist; i++) {
				cureTurn.Add(Action(DRIVE, drive_path.nodes[i], currentPlayer, currentPlayer));
			}
			cureTurn.Add(MakeCureAction(cureColor));
			list.Add(cureTurn);
		}

		// Now we try to find the fastest way to station.
		// 1.2. Can we build a station in the city we are standing on
		// by using Government Grant
		if (!cityState.HasStation(currentCity) && players.DoPlayersHaveEventCard(EventCardID::GovGrant)) {
			if (actions_left >= 1) {
				cureTurn.Clear();
				uint8_t owner = players.GetOwnerOf(EventCardID::GovGrant);
				cureTurn.Add(Action(GOVERNMENT_GRANT, currentCity, owner, owner));
				cureTurn.Add(MakeCureAction(cureColor));
				list.Add(cureTurn);
			}
		}

		// 1.3. Can we build a station in the city we are standing on.
		// by using a card (need to check if we don't need the card for the cure)
		if (!cityState.HasStation(currentCity) &&
			players.HasCard(currentPlayer, currentCity) &&
			!players.IsNeededForCure(currentPlayer, cureColor, currentCity))
		{
			if (actions_left >= 2) {
				cureTurn.Clear();
				cureTurn.Add(Action(BUILD, currentCity, currentPlayer, currentPlayer));
				cureTurn.Add(MakeCureAction(cureColor));
				list.Add(cureTurn);
			}
		}

		// 1.4. Check the fastest path to every station
		cureTurn.Clear();
		if (GetFastestPathToAnyStation(
			currentPlayer,
			true,
			cureColor,
			(players.HasRole(currentPlayer, Role::Scientist)) ? 4 : 5,
			cureTurn,
			gameFlags.GetActionsRemaining()))
		{
			if (cureTurn.count < actions_left) {
				cureTurn.Add(MakeCureAction(cureColor));
			}
			list.Add(cureTurn);
		}
	}
}

void GameState::AddShareTurns(TurnList& list) const
{
	uint8_t activePlayer = gameFlags.GetActivePlayer();
	uint8_t activeCity = players.GetLocation(activePlayer);
	int actions_left = gameFlags.GetActionsRemaining();
	Role activeRole = players.GetRole(activePlayer);

	// Helper to safely instantiate SHARE without constructor ambiguity,
	// enforcing the struct layout cleanly.
	auto MakeShareAction = [](uint8_t giver, uint8_t receiver, uint8_t cardToShare, bool isGiving) {
		Action a;
		a.base.type = SHARE;
		a.share.player_id = giver;
		a.share.receiving_player_id = receiver;
		a.share.target_city = cardToShare;
		a.share.is_giving = isGiving;
		return a;
	};

	// PRE-CALCULATE CURERS: Array mapping ColorType (0-3) to a Player ID
	uint8_t designatedCurers[ColorType::COUNT];
	for (int c = 0; c < ColorType::COUNT; c++) {
		designatedCurers[c] = GetDesignatedCurer(static_cast<ColorType>(c));
	}

	for (int otherPlayer = 0; otherPlayer < players.count; otherPlayer++) {
		if (otherPlayer == activePlayer) continue;

		uint8_t otherCity = players.GetLocation(otherPlayer);
		Role otherRole = players.GetRole(otherPlayer);

		// 1. ACTIVE GIVES TO PASSIVE
		uint64_t activeHand = players.hands[activePlayer];
		while (activeHand > 0) {
			uint8_t cardId = std::countr_zero(activeHand);
			activeHand &= (activeHand - 1);

			if (CardRegistry::IsEvent(cardId)) continue;
			ColorType cardColor = CardRegistry::GetColor(cardId);

			// FILTER 1: Is the passive player the DESIGNATED CURER for this color?
			if (designatedCurers[cardColor] == otherPlayer) {

				uint8_t rendezvous = (activeRole == Role::Researcher) ? otherCity : cardId;
				Turn shareMacro;

				if (GetFastestPath(activePlayer, rendezvous, true, ColorType::NO_COLOR, 0, shareMacro, actions_left)) {

					// If we made it to the rendezvous AND the other player is waiting there
					if (rendezvous == otherCity && shareMacro.count < actions_left) {
						// Active builds path, then actively gives it away
						shareMacro.Add(MakeShareAction(activePlayer, otherPlayer, cardId, true));
					}

					// We add the macro EVEN IF the share wasn't added. 
					// This allows the AI to "walk towards the rendezvous" as a valid turn.
					if (shareMacro.count > 0) {
						list.Add(shareMacro);
					}
				}
			}
		}

		// 2. ACTIVE TAKES FROM PASSIVE
		uint64_t passiveHand = players.hands[otherPlayer];
		while (passiveHand > 0) {
			uint8_t cardId = std::countr_zero(passiveHand);
			passiveHand &= (passiveHand - 1);

			if (CardRegistry::IsEvent(cardId)) continue;
			ColorType cardColor = CardRegistry::GetColor(cardId);

			// FILTER 1: Are WE the DESIGNATED CURER for this color?
			if (designatedCurers[cardColor] == activePlayer) {

				uint8_t rendezvous = (otherRole == Role::Researcher) ? otherCity : cardId;
				Turn shareMacro;

				if (GetFastestPath(activePlayer, rendezvous, true, ColorType::NO_COLOR, 0, shareMacro, actions_left)) {

					// If we made it to the rendezvous AND the other player is waiting there
					if (rendezvous == otherCity && shareMacro.count < actions_left) {
						// Active builds path, then actively takes it
						shareMacro.Add(MakeShareAction(otherPlayer, activePlayer, cardId, false));
					}

					// Again, add the macro so the AI can evaluate "getting closer" as a good move.
					if (shareMacro.count > 0) {
						list.Add(shareMacro);
					}
				}
			}
		}
	}
}

void GameState::AddTreatTurns(TurnList& list) const
{
	uint8_t currentPlayer = gameFlags.GetActivePlayer();
	uint8_t currentCity = players.GetLocation(currentPlayer);
	int actions_left = gameFlags.GetActionsRemaining();
	Role role = players.GetRole(currentPlayer);

	// HELPER: How many treats can we squeeze in?
	auto AppendTreats = [&](Turn& macro, uint8_t target_city, ColorType color, int available_ap) {
		if (available_ap <= 0) return;

		int cubes_present = cityState.GetCubeCount(target_city, color);

		// The Medic treats ALL cubes of a color for 1 action
		// and for 0 actions if the color is cured.
		// A normal role needs 1 action per cube.
		// Also, ANY role treats all cubes for 1 action IF the disease is cured.
		bool is_mass_treat = (role == Role::Medic) || gameFlags.IsCured(color);
		bool is_passive_treat = (role == Role::Medic) && gameFlags.IsCured(color);

		int treats_needed = is_mass_treat ? 1 : cubes_present;
		treats_needed = is_passive_treat ? 0 : treats_needed;
		int treats_to_do = std::min(treats_needed, available_ap);

		for (int i = 0; i < treats_to_do; i++) {
			macro.Add(Action(TREAT, target_city, currentPlayer, color));
		}
		};

	// 1. TREAT CURRENT CITY (Priority 0)
	// If we are standing on cubes, we should almost always evaluate treating them.
	for (int c = 0; c < 4; c++) {
		ColorType color = static_cast<ColorType>(c);
		if (cityState.HasDisease(currentCity, color)) {
			Turn treatTurn;
			AppendTreats(treatTurn, currentCity, color, actions_left);
			list.Add(treatTurn);
		}
	}

	// 2. TRAVEL AND TREAT (High-Priority Targets)
	// We want to extract Hotspots (3 cubes) and Danger Zones (2 cubes).
	uint64_t target_mask = cityState.GetHotspotMask();

	// Add 2-cube cities to the target mask
	for (uint8_t i = 0; i < NUMBER_OF_CITIES; i++) {
		if (cityState.GetCubeCount(i) == 2) {
			target_mask |= (1ULL << i);
		}
	}

	// Remove current city from the mask
	target_mask &= ~(1ULL << currentCity);

	// Loop through the high-priority targets
	while (target_mask > 0) {
		uint8_t target_city = std::countr_zero(target_mask);

		Turn pathMacro;

		if (GetFastestPath(currentPlayer, target_city, false, ColorType::NO_COLOR, 0, pathMacro, actions_left)) {

			if (pathMacro.count < actions_left) {
				ColorType dominantColor = cityState.GetDominantDiseaseColor(target_city);

				int remaining_ap = actions_left - pathMacro.count;
				AppendTreats(pathMacro, target_city, dominantColor, remaining_ap);
			}

			list.Add(pathMacro);
		}

		target_mask &= (target_mask - 1);
	}
}

void GameState::AddBuildTurns(TurnList& list) const
{
	uint8_t currentPlayer = gameFlags.GetActivePlayer();
	uint8_t currentCity = players.GetLocation(currentPlayer);
	int actions_left = gameFlags.GetActionsRemaining();
	Role role = players.GetRole(currentPlayer);

	const int MIN_STATION_DISTANCE = 3;

	ColorType cureColor = players.GetCureColor(currentPlayer);
	int cureThreshold = (role == Role::Scientist) ? 4 : 5;

	// 1. OPERATIONS EXPERT (The Free Build)
	if (role == Role::Operations) {
		// If there is no station around
		if (!cityState.HasStation(currentCity) &&
			cityState.GetDistanceToNearestStation(currentCity) >= MIN_STATION_DISTANCE)
		{
			Turn opsBuildTurn;
			// Takes 1 action to build
			if (actions_left >= 1) {
				opsBuildTurn.Add(Action(EXPERT_BUILD, currentCity, currentPlayer, currentPlayer));
				list.Add(opsBuildTurn);
			}
		}
		// Note: We don't generate "Walk towards empty cities to build" for the Ops Expert
		// here, because that would generate 48 different branches! Their standard driving 
		// handles their movement, and they will naturally build when they land in a good spot.
	}

	// 2. STANDARD BUILD (Using City Cards)
	uint64_t temp_hand = players.hands[currentPlayer];

	while (temp_hand > 0) {
		uint8_t target_city = std::countr_zero(temp_hand);
		temp_hand &= (temp_hand - 1);

		if (CardRegistry::IsEvent(target_city)) continue;

		// FILTER 1: Is there already a station there?
		if (cityState.HasStation(target_city)) continue;

		// FILTER 2: Is it too close to an existing station?
		if (cityState.GetDistanceToNearestStation(target_city) < MIN_STATION_DISTANCE) continue;

		// FILTER 3: Do we desperately need this card for a cure?
		if (players.IsNeededForCure(currentPlayer, cureColor, target_city)) continue;

		Turn buildMacro;

		// Note: prioritize_cards = true. We'd rather walk and save flight cards for emergencies.
		if (GetFastestPath(currentPlayer, target_city, false, cureColor, cureThreshold, buildMacro, actions_left, target_city)) {

			// If the path length is strictly LESS than our available actions, 
			// it means we arrived with at least 1 action left to actually drop the station.
			if (buildMacro.count < actions_left) {
				buildMacro.Add(Action(BUILD, target_city, currentPlayer, currentPlayer));
			}

			list.Add(buildMacro);
		}
	}
}

void GameState::AddWalkTurn(TurnList& list) const
{
	int actions_left = gameFlags.GetActionsRemaining();
	if (actions_left <= 0) return;

	uint8_t currentPlayer = gameFlags.GetActivePlayer();
	uint8_t currentCity = players.GetLocation(currentPlayer);

	// Just add 1 macro for every adjacent city. 
	int count = MapData::neighbor_counts[currentCity];
	for (int i = 0; i < count; i++) {
		Turn driveTurn;
		driveTurn.Add(Action(DRIVE, MapData::adjacency[currentCity][i], currentPlayer, currentPlayer));
		list.Add(driveTurn);
	}
}

void GameState::HandleLimits()
{
	int amount;
	for (int player_id = 0; player_id < players.count; player_id++) {
		int hand_size = players.GetHandSize(player_id);
		if (hand_size > HAND_LIMIT) {
			amount = hand_size - HAND_LIMIT;
			for (int i = 0; i < amount; i++) {
				// GetBestCardToDiscard protects cure cards & checks tactical value
				uint8_t worst_card = GetBestCardToDiscard(player_id);
				if (worst_card != 255) {
					// This properly routes the card to the discard pile
					DoDiscardPlayerCard(player_id, worst_card);
				}
			}
		}
	}

	int station_count = cityState.GetStationCount();
	if (station_count > MAX_RESEARCH_LAB_COUNT) {
		amount = station_count - MAX_RESEARCH_LAB_COUNT;
		for (int i = 0; i < amount; i++) {
			// GetTheWorstStation uses MapData to find the most redundant station
			uint8_t worst_station = GetTheWorstStation();
			if (worst_station != 255) {
				DoRemoveStation(worst_station);
			}
		}
	}
}

void GameState::Execute(Action action)
{
	switch (action.base.type) {

	case DRIVE:
		DoDrive(action.move.target_city, action.move.target_player_id);
		gameFlags.UseAction();
		break;

	case DIRECT_FLIGHT:
		DoDirectFlight(action.move.target_city, action.move.executing_player_id, action.move.target_player_id);
		gameFlags.UseAction();
		break;

	case CHARTER_FLIGHT:
		DoCharterFlight(action.move.target_city, action.move.executing_player_id, action.move.target_player_id);
		gameFlags.UseAction();
		break;

	case SHUTTLE_FLIGHT:
		DoShuttleFlight(action.move.target_city, action.move.target_player_id);
		gameFlags.UseAction();
		break;

	case BUILD:
		DoBuild();
		gameFlags.UseAction();
		break;

	case TREAT:
		DoTreat((ColorType)action.treat.color_id);
		gameFlags.UseAction();
		break;

	case SHARE:
		DoShare(action.share.player_id, action.share.receiving_player_id, action.share.target_city);
		gameFlags.UseAction();
		break;

	case CURE:
		if (action.discover_cure.color_card1_id == 15 &&
			action.discover_cure.color_card2_id == 15 &&
			action.discover_cure.color_card3_id == 15 &&
			action.discover_cure.color_card4_id == 15)
		{
			DoSmartDiscover(gameFlags.GetActivePlayer(), (ColorType)action.discover_cure.color_id);
		}
		else {
			DoDiscover(
				action.discover_cure.color_card0_id + action.discover_cure.color_id * NUMBER_OF_CITIES_PER_COLOR,
				action.discover_cure.color_card1_id + action.discover_cure.color_id * NUMBER_OF_CITIES_PER_COLOR,
				action.discover_cure.color_card2_id + action.discover_cure.color_id * NUMBER_OF_CITIES_PER_COLOR,
				action.discover_cure.color_card3_id + action.discover_cure.color_id * NUMBER_OF_CITIES_PER_COLOR,
				action.discover_cure.color_card4_id + action.discover_cure.color_id * NUMBER_OF_CITIES_PER_COLOR
			);
		}
		gameFlags.UseAction();
		break;

	case GOVERNMENT_GRANT:
		DoGovernmentGrant(action.move.executing_player_id, action.move.target_city);
		break;

	case FORECAST:
		DoForecast(
			action.forecast.player_id,
			action.forecast.card_index0,
			action.forecast.card_index1,
			action.forecast.card_index2,
			action.forecast.card_index3,
			action.forecast.card_index4,
			action.forecast.card_index5
		);
		break;

	case RESILIENT_POPULATION:
		DoResilientPopulation(action.move.executing_player_id, action.move.target_city);
		break;

	case ONE_QUIET_NIGHT:
		DoOneQuietNight(action.move.executing_player_id);
		break;

	case AIRLIFT:
		DoAirlift(action.move.executing_player_id, action.move.target_player_id, action.move.target_city);
		break;

	case PLANNER_TAKE:
		DoContingencyPlannerTake(action.move.target_city);
		gameFlags.UseAction();
		break;

	case DISPATCHER_MOVE:
		DoDispatcher(action.move.target_player_id, action.move.target_city);
		gameFlags.UseAction();
		break;

	case DISPATCHER_MOVE_AS:
		DoDispatcher(action.move.target_player_id, action.move.target_city);
		gameFlags.UseAction();
		break;

	case EXPERT_BUILD:
		DoOperationsExpertBuild();
		gameFlags.UseAction();
		break;

	case EXPERT_MOVE:
		DoOperationsExpertMovement(action.ops_expert.target_city, action.ops_expert.discard_city);
		gameFlags.UseAction();
		break;

	case DISCARD_CARD:
		DoDiscardPlayerCard(action.move.executing_player_id, action.move.target_city);
		break;

	case REMOVE_STATION:
		DoRemoveStation(action.move.target_city);
		break;

	case END_TURN:
		gameFlags.SetActionsRemaining(0);
		break;

	default:
		assert(false && "Unhandled action type in Execute!");
		break;
	}

	UpdateEradicationFlag();

	// ===== Game End checks ===== 

	// The players win as soon as cures to all 4 diseases are discovered
	if (gameFlags.IsAllCured()) {
		currentState = State::AllCured;
	}

	// if the outbreaks marker reaches the last space of the Outbreaks Track
	else if (gameFlags.GetOutbreaks() == OUTBREAK_MARKER_MAX) {
		currentState = State::OutbreakMarkerMaxed;
	}

	// if you are unable to place the number of disease cubes actually
	// needed on the board
	else if (cityState.HasLostToCubes()) {
		currentState = State::NoMoreDiseaseCubes;
	}

	// if a player cannot draw 2 Player cards after doing his actions.
	else if (decks.player_deck.Count() < 2) {
		currentState = State::NotEnoughPlayerCards;
	}

	EndTurn();
}

void GameState::HandleOutbreak(uint8_t city_id, ColorType color) 
{
	cityState.SetOutbroken(city_id);

	gameFlags.IncOutbreaks();
	if (gameFlags.GetOutbreaks() >= OUTBREAK_MARKER_MAX) {
		// GAME OVER
		currentState = State::OutbreakMarkerMaxed;
		return;
	}

	// Spread to neighbors
	int count = MapData::GetNeighborCount(city_id);
	const uint8_t* neighbors = MapData::GetNeighbors(city_id);

	for (int i = 0; i < count; i++) {
		InfectCity(neighbors[i], color); // Recursive call
	}
}

uint8_t GameState::GetTheWorstStation() const
{
	/*
	Used for removing one of the stations if we
	exceed the station limit.
	*/

	uint64_t station_mask = cityState.GetStationMask();
	uint64_t temp_mask = station_mask;

	uint8_t worst_station_id = 255;
	uint8_t worst_min_dist = 255;

	// Iterate through each candidate station
	while (temp_mask > 0) {
		uint8_t candidate_id = std::countr_zero(temp_mask);

		// Create a mask of ALL OTHER stations (turn off the candidate's bit)
		uint64_t others_mask = station_mask & ~(1ULL << candidate_id);

		uint8_t dist_to_nearest = MapData::GetDistanceToNearest(candidate_id, others_mask);

		if (dist_to_nearest < worst_min_dist) {
			worst_min_dist = dist_to_nearest;
			worst_station_id = candidate_id;
		}

		temp_mask &= (temp_mask - 1);
	}

	return worst_station_id;
}

uint8_t GameState::GetBestCardToDiscard(uint8_t player_id) const
{
	uint64_t hand = players.hands[player_id];
	if (hand == 0) return 255;

	// Get the player's set collection status
	auto mostFrequent = players.GetMostFrequentColor(player_id);
	uint8_t requiredForCure = (players.roles[player_id] == (uint8_t)Role::Scientist) ? 4 : 5;

	uint8_t best_card = 255;
	float lowest_score = std::numeric_limits<float>::max();

	while (hand > 0) {
		int city_id = std::countr_zero(hand);
		ColorType color = CardRegistry::GetColor(city_id);
		float score = 0.0f;

		// --- 1. SET PROTECTION LOGIC ---
		int currentCount = players.GetColorCount(player_id, color);

		if (gameFlags.IsCured(color)) {
			// Cured cards are the best candidates to discard
			score -= 100.0f;
		}
		else {
			// Protect cards if the player is close to a cure (e.g., has 3+ cards of that color)
			// The closer to the goal, the exponentially more we want to keep it
			if (currentCount >= 3) {
				score += (currentCount * 25.0f);
			}

			// Specifically protect the "Most Frequent" color even more
			if (color == mostFrequent.color) {
				score += 10.0f;
			}
		}

		// --- 2. TACTICAL VALUE (CUBES) ---
		// If a city is dangerous (3 cubes), keep the card for Direct Flight/Treat
		int cubes = cityState.GetTotalCubeCount(city_id);
		score += (cubes * 15.0f);

		// --- 3. SELECTION ---
		if (score < lowest_score) {
			lowest_score = score;
			best_card = (uint8_t)city_id;
		}

		hand &= (hand - 1);
	}

	return best_card;
}

std::vector<uint8_t> GameState::GetBestCardsForCure(uint8_t player_id, ColorType color) const
{
	int required = (players.roles[player_id] == (uint8_t)Role::Scientist) ? 4 : 5;

	// Create a mask of the player's hand filtered by color
	uint64_t color_hand = players.hands[player_id] & GameConstants::COLOR_MASKS[(int)color];

	struct CardScore { uint8_t id; float score; };
	std::vector<CardScore> candidates;
	candidates.reserve(HAND_LIMIT);

	while (color_hand > 0) {
		int city_id = std::countr_zero(color_hand);

		float score = 0.0f;
		// We prefer to KEEP cards with high cube counts (for direct flights to treat)
		score += (cityState.GetTotalCubeCount(city_id) * 10.0f);

		// We prefer to KEEP cards for cities far from existing research stations
		score += (cityState.GetDistanceToNearestStation(city_id) * 2.0f);

		candidates.push_back({ (uint8_t)city_id, score });
		color_hand &= (color_hand - 1);
	}

	// Sort: Lowest scores (safest/least useful cities) are used for the cure first
	std::sort(candidates.begin(), candidates.end(), [](const CardScore& a, const CardScore& b) {
		return a.score < b.score;
		});

	std::vector<uint8_t> result;
	for (int i = 0; i < required && i < (int)candidates.size(); ++i) {
		result.push_back(candidates[i].id);
	}
	return result;
}

bool GameState::GetFastestPath(uint8_t player_id, uint8_t target_city, bool use_events, ColorType protected_color, int protected_threshold, Turn& out_path, int action_count, uint8_t excluded_card) const
{
	out_path.Clear();
	uint8_t start_city = players.GetLocation(player_id);

	if (start_city == target_city) return true;

	Turn best_path;
	int best_cost = 99; // 99 acts as "infinity" for Pandemic distances

	// HELPER: Evaluates path and stores it if it's the new fastest
	auto TryUpdatePath = [&](int total_cost, bool has_special, const Action& special_action, uint8_t intermediate_city) {
		if (total_cost < best_cost) {
			best_cost = total_cost;
			best_path.Clear();

			if (has_special) {
				best_path.Add(special_action);
			}

			const StaticPath& remaining_drive = MapData::drivePaths[intermediate_city][target_city];
			for (int i = 0; i < remaining_drive.length; i++) {
				best_path.Add(Action(DRIVE, remaining_drive.nodes[i], player_id, player_id));
			}
		}
		};

	// 1. BASELINE: Just Driving
	const StaticPath& drivePath = MapData::drivePaths[start_city][target_city];
	TryUpdatePath(drivePath.length, false, Action(), start_city);

	// 2. EVENT CARDS (Cost: 0 Actions)
	if (use_events) {
		if (players.DoPlayersHaveEventCard(EventCardID::Airlift)) {
			// Airlift is an instant teleport. Best possible path.
			uint8_t airlift_owner = players.GetOwnerOf(EventCardID::Airlift);
			out_path.Add(Action(AIRLIFT, target_city, airlift_owner, player_id));
			return true;
		}
	}

	// 3. ROLE ABILITIES
	if (players.GetRole(player_id) == Role::Dispatcher) {
		for (int p = 0; p < players.count; p++) {
			if (p == player_id) continue;
			uint8_t friend_loc = players.GetLocation(p);
			int cost = 1 + MapData::drivePaths[friend_loc][target_city].length;

			TryUpdatePath(cost, true, Action(DISPATCHER_MOVE, friend_loc, player_id, player_id), friend_loc);
		}
	}
	else if (players.GetRole(player_id) == Role::Operations) {
		if (cityState.HasStation(start_city)) {
			uint64_t temp_hand = players.hands[player_id];
			uint8_t trash_card = 255;

			while (temp_hand > 0) {
				uint8_t cardId = std::countr_zero(temp_hand);
				temp_hand &= (temp_hand - 1);

				if (cardId == excluded_card) continue;

				if (!CardRegistry::IsEvent(cardId) && !players.IsNeededForCure(player_id, protected_color, cardId)) {
					trash_card = cardId;
					break;
				}
			}

			if (trash_card != 255) {
				TryUpdatePath(1, true, Action(EXPERT_MOVE, target_city, trash_card, player_id, player_id), target_city);
			}
		}
	}

	// 4. SHUTTLE FLIGHTS
	if (cityState.HasStation(start_city)) {
		uint64_t stations = cityState.GetStationMask() & ~(1ULL << start_city);
		while (stations > 0) {
			uint8_t station_city = std::countr_zero(stations);
			int cost = 1 + MapData::drivePaths[station_city][target_city].length;

			TryUpdatePath(cost, true, Action(SHUTTLE_FLIGHT, station_city, player_id, player_id), station_city);
			stations &= (stations - 1);
		}
	}

	// 5. DIRECT & CHARTER FLIGHTS
	uint64_t temp_hand = players.hands[player_id];
	while (temp_hand > 0) {
		uint8_t cardId = std::countr_zero(temp_hand);
		temp_hand &= (temp_hand - 1);

		if (cardId == excluded_card) continue;

		if (!CardRegistry::IsEvent(cardId) && !players.IsNeededForCure(player_id, protected_color, cardId)) {

			if (cardId == start_city) {
				TryUpdatePath(1, true, Action(CHARTER_FLIGHT, target_city, player_id, player_id), target_city);
			}
			else {
				int cost = 1 + MapData::drivePaths[cardId][target_city].length;
				TryUpdatePath(cost, true, Action(DIRECT_FLIGHT, cardId, player_id, player_id), cardId);
			}
		}
	}

	// 6. FINALIZE
	if (best_cost < 99) {
		int actions_to_take = std::min(best_path.count, (uint8_t)action_count);

		for (int i = 0; i < actions_to_take; i++) {
			out_path.Add(best_path.actions[i]);
		}
		return true;
	}

	// Should not happen.
	// There is always a way <99 to the location using only drive
	return false;
}

bool GameState::GetFastestPathToAnyStation(uint8_t player_id, bool use_events, ColorType protected_color, int protected_threshold, Turn& out_path, int action_count) const
{
	out_path.Clear();

	// 1. Get all active research stations
	uint64_t stations = cityState.GetStationMask();
	if (stations == 0) return false;

	Turn best_overall_path;
	int shortest_path_length = 99;

	// 2. Loop through every station
	while (stations > 0) {
		uint8_t station_city = std::countr_zero(stations);

		Turn temp_path;

		if (GetFastestPath(player_id, station_city, use_events, protected_color, protected_threshold, temp_path, action_count)) {

			// The player is already at a station
			if (temp_path.count == 0) {
				out_path.Clear();
				return true;
			}

			if (temp_path.count < shortest_path_length) {
				shortest_path_length = temp_path.count;
				best_overall_path = temp_path;
			}
		}

		stations &= (stations - 1);
	}

	if (shortest_path_length < 99) {
		out_path = best_overall_path;
		return true;
	}

	return false;
}

uint8_t GameState::GetDesignatedCurer(ColorType color) const
{
	uint8_t bestPlayer = 255;
	int bestScore = -1;

	if (gameFlags.IsCured(color)) return bestPlayer;

	for (uint8_t p = 0; p < players.count; p++) {
		int cardCount = 0;
		uint64_t temp_hand = players.hands[p];

		while (temp_hand > 0) {
			uint8_t cardId = std::countr_zero(temp_hand);
			temp_hand &= (temp_hand - 1);

			if (!CardRegistry::IsEvent(cardId) && CardRegistry::GetColor(cardId) == color) {
				cardCount++;
			}
		}

		// Scoring: 10 points per card. 
		// The Scientist needs 1 less card, effectively giving them a massive head start.
		int score = cardCount * 10;
		if (players.GetRole(p) == Role::Scientist) {
			score += 5;
		}

		// If a player holds 0 cards, they shouldn't be the designated curer 
		// unless literally no one has cards of this color.
		if (cardCount == 0 && bestScore > -1) continue;

		// Tie-breaker uses Player ID to ensure deterministic behavior (prevents oscillation)
		if (score > bestScore || (score == bestScore && p < bestPlayer)) {
			bestScore = score;
			bestPlayer = p;
		}
	}

	return bestPlayer;
}

std::vector<float> GameState::ToTensor() const
{
	std::vector<float> obs;
	obs.reserve(700);

	// 1. City Features (48 cities)
	for (int city_id = 0; city_id < NUMBER_OF_CITIES; city_id++) {
		// Disease cubes (0-3 scale to 0.0-1.0)
		obs.push_back(cityState.GetCubeCount(city_id, ColorType::BLUE) / 3.0f);
		obs.push_back(cityState.GetCubeCount(city_id, ColorType::YELLOW) / 3.0f);
		obs.push_back(cityState.GetCubeCount(city_id, ColorType::BLACK) / 3.0f);
		obs.push_back(cityState.GetCubeCount(city_id, ColorType::RED) / 3.0f);

		// Research Station (Boolean)
		obs.push_back(cityState.HasStation(city_id) ? 1.0f : 0.0f);
	}

	// 2. Player Locations (48 cities * 4 players)
	// One-hot encoding of where each player is standing
	for (int p = 0; p < NUMBER_OF_MAX_PLAYERS; p++) {
		uint8_t player_location = players.GetLocation(p);
		for (int city_id = 0; city_id < NUMBER_OF_CITIES; city_id++) {
			obs.push_back(player_location == city_id ? 1.0f : 0.0f);
		}
	}

	// 3. Player Hands (48 city + 5 event * 4 players)
	// One-hot encoding of who owns which city card
	for (int p = 0; p < NUMBER_OF_MAX_PLAYERS; p++) {
		for (int city_id = 0; city_id < NUMBER_OF_CITIES + NUMBER_OF_EVENT_CARDS; city_id++) {
			obs.push_back(players.HasCard(p, city_id) ? 1.0f : 0.0f);
		}
	}

	// 4. Global Game Status (Normalized)
	obs.push_back(gameFlags.GetInfectionRateIndex() / 6.0f); // Max index is 6
	obs.push_back(gameFlags.GetOutbreaks() / 8.0f);           // 8 is game over

	// Cures (4 bits)
	obs.push_back(gameFlags.IsCured(ColorType::BLUE) ? 1.0f : 0.0f);
	obs.push_back(gameFlags.IsCured(ColorType::YELLOW) ? 1.0f : 0.0f);
	obs.push_back(gameFlags.IsCured(ColorType::BLACK) ? 1.0f : 0.0f);
	obs.push_back(gameFlags.IsCured(ColorType::RED) ? 1.0f : 0.0f);

	// 5. Current Player Identity
	// Very important so the AI knows "Who am I right now?"
	for (int p = 0; p < NUMBER_OF_MAX_PLAYERS; p++) {
		obs.push_back(gameFlags.GetActivePlayer() == p ? 1.0f : 0.0f);
	}

	// Player Roles (One-hot for active player)
	for (int p = 0; p < NUMBER_OF_MAX_PLAYERS; p++) {
		for (int r = 0; r < NUMBER_OF_ROLE_CARDS; r++) {
			obs.push_back((int)players.GetRole(p) == r ? 1.0f : 0.0f);
		}
	}

	return obs;
}
