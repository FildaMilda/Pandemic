#include "Game.h"

void GameState::Setup(std::mt19937* rng)
{
	this->rng_ptr = rng;
	currentState = State::InProgress;

	gameFlags.Init();
	cityState.Init();
	players.Init(4);
	decks.infection_deck.Init(rng);
	decks.player_deck.Init(rng);

	InfectCitiesSetup();
	DealPlayerCards();
	InsertEpidemicCards(rng, Difficulty::HEROIC);
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

		void GenerateCureCombinations(int required_to_cure, Color cure_color, ActionList& list) {
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
			Color color = CardRegistry::GetColor(card);

			switch (color) {
			case Color::BLACK:
				black_cards.Add(card);
				break;
			case Color::BLUE:
				blue_cards.Add(card);
				break;
			case Color::RED:
				red_cards.Add(card);
				break;
			case Color::YELLOW:
				yellow_cards.Add(card);
				break;
			}
		}

		void GenerateCureCombinations(int required_to_cure, ActionList& list) {
			red_cards.GenerateCureCombinations(required_to_cure, Color::RED, list);
			blue_cards.GenerateCureCombinations(required_to_cure, Color::BLUE, list);
			black_cards.GenerateCureCombinations(required_to_cure, Color::BLACK, list);
			yellow_cards.GenerateCureCombinations(required_to_cure, Color::YELLOW, list);
		}
	};

	CardCounter cure_counter;

	// Player don't have to use all 4 actions
	// End turn action added
	list.Add(END_TURN);

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

			// Build Station
			if (cardId == currentCity) {
				list.Add(BUILD, currentCity, currentPlayer, currentPlayer);
				canCharter = true;
			}

			// Direct Flight
			if (!CardRegistry::IsEvent(cardId))
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
		break;

	case Role::Dispatcher:
		AddDispatcherActions(list);
		break;

	case Role::Operations:
		AddOpsExpertActions(list);
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
		/*
		for (int player_id = 0; player_id < players.count; player_id++) {
			for (int city_id = 0; city_id < NUMBER_OF_CITIES; city_id++) {
				if (players.GetLocation(player_id) != city_id) {
					list.Add(AIRLIFT, city_id, card_owner_id, player_id);
				}
			}
		}
		*/
		break;
	}
}

void GameState::AddDispatcherActions(ActionList& list) const
{
}

void GameState::AddOpsExpertActions(ActionList& list) const
{
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
		DoTreat((Color)action.treat.color_id);
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
		DoGovernmentGrant(action.move.player_id, action.move.target_city);
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
		DoResilientPopulation(action.move.player_id, action.move.target_city);
		break;

	case ONE_QUIET_NIGHT:
		DoOneQuietNight(action.move.player_id);
		break;

	case AIRLIFT:
		DoAirlift(action.move.player_id2, action.move.player_id, action.move.target_city);
		break;

	case PLANNER_TAKE:
		DoContingencyPlannerTake(action.move.target_city);
		gameFlags.UseAction();
		break;

	case DISPATCHER_MOVE:
		DoDispatcher(action.move.player_id, action.move.target_city);
		gameFlags.UseAction();
		break;

	case DISPATCHER_MOVE_AS:
		DoDispatcher(action.move.player_id, action.move.target_city);
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
		DoDiscardPlayerCard(action.move.player_id, action.move.target_city);
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

void GameState::HandleOutbreak(uint8_t city_id, Color color) 
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
