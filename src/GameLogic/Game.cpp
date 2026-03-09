#include "Game.h"

void GameState::Setup(Difficulty diff, uint8_t player_count, std::mt19937* rng)
{
	this->rng_ptr = rng;
	currentState = State::InProgress;

	gameFlags.Init();
	cityState.Init();
	players.Init(player_count);
	decks.infection_deck.Init(rng);
	decks.player_deck.Init(rng);

	InfectCitiesSetup();
	DealPlayerCards();
	InsertEpidemicCards(rng, diff);
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

void GameState::InsertEpidemicCards(std::mt19937* rng, Difficulty diff)
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
		int randomIndex = dist(*rng);

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
		int rand_idx = dist(*rng_ptr);

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
		if (!gameFlags.IsContingencyPlannerSlotEmpty()) {
			AddEventAction(list, gameFlags.GetContingencyPlannerSlot(), currentPlayer);
		}
		// TODO: Add action to TAKE the card
		break;

	case Role::Dispatcher:
		list.Add(DISPATCHER_MOVE);
		//AddDispatcherActions(list);
		break;

	case Role::Operations:
		list.Add(EXPERT_MOVE);
		//AddOpsExpertActions(list);
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
				AddEventAction(list, cardId, currentPlayer);
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
			AddEventAction(list, gameFlags.GetContingencyPlannerSlot(), currentPlayer);
		}
		//TODO add the TAKE action
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
		// TODO
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

void GameState::AddDispatcherActions(ActionList& list) const
{
	// The Dispatcher may, as an action, either:
	
	// Move any pawn, to any city
	// containing another pawn
	for (int player_a = 0; player_a < players.count; player_a++) {
		for (int player_b = 0; player_b < players.count; player_b++) {
			if (player_a == player_b) continue;

			// Moving player A to player B
			list.Add(DISPATCHER_MOVE,
				players.GetLocation(player_b),
				gameFlags.GetActivePlayer(),
				player_a
			);
		}
	}

	// Move another player’s pawn,
	// as if it were his own
	// TODO
}

void GameState::AddOpsExpertActions(ActionList& list) const {
	// TODO
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
			list.Add(DRIVE, *neighbors, pawn_to_move, currentPlayer);
			neighbors++;
		}

		// 2B. SHUTTLE FLIGHT (Station to Station)
		if (cityState.HasStation(pawn_loc)) {
			uint64_t stations = cityState.GetStationMask() & ~(1ULL << pawn_loc);
			while (stations > 0) {
				list.Add(SHUTTLE_FLIGHT, std::countr_zero(stations), pawn_to_move, currentPlayer);
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
						list.Add(DIRECT_FLIGHT, cardId, pawn_to_move, currentPlayer);
					}
				}

				// CHARTER FLIGHT: Dispatcher discards card matching pawn's current location
				if (cardId == pawn_loc) {
					uint64_t target_mask = cityState.GetStationMask() | cityState.GetHotspotMask();

					// Add cities with other players
					for (int p = 0; p < players.count; p++) target_mask |= (1ULL << players.GetLocation(p));

					target_mask &= ~(1ULL << pawn_loc); // Remove current location

					while (target_mask > 0) {
						list.Add(CHARTER_FLIGHT, std::countr_zero(target_mask), pawn_to_move, currentPlayer);
						target_mask &= (target_mask - 1);
					}
				}
			}
			temp_hand &= (temp_hand - 1);
		}
	}
}

void GameState::Execute(Action action)
{
	switch (action.base.type) {

	case DRIVE:
		DoDrive(action.move.target_city);
		gameFlags.UseAction();
		break;

	case DIRECT_FLIGHT:
		DoDirectFlight(action.move.target_city);
		gameFlags.UseAction();
		break;

	case CHARTER_FLIGHT:
		DoCharterFlight(action.move.target_city);
		gameFlags.UseAction();
		break;

	case SHUTTLE_FLIGHT:
		DoShuttleFlight(action.move.target_city);
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
		DoShare(action.share.receiving_player_id, action.share.target_city);
		gameFlags.UseAction();
		break;

	case CURE:
		DoDiscover(
			action.discover_cure.color_card0_id,
			action.discover_cure.color_card1_id,
			action.discover_cure.color_card2_id,
			action.discover_cure.color_card3_id,
			action.discover_cure.color_card4_id
		);
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
		DoOperationsExpertMovement(action.move.target_city);
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
	if (gameFlags.GetOutbreaks() == OUTBREAK_MARKER_MAX) {
		currentState = State::OutbreakMarkerMaxed;
	}

	// if you are unable to place the number of disease cubes actually
	// needed on the board
	if (cityState.HasLostToCubes()) {
		currentState = State::NoMoreDiseaseCubes;
	}

	// if a player cannot draw 2 Player cards after doing his actions.
	if (decks.player_deck.Count() < 2) {
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
