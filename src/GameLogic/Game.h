#ifndef GAME_H
#define GAME_H

#include "City.h"
#include "Player.h"
#include "Deck.h"
#include "Flags.h"
#include "Map.h"
#include "Action.h"
#include "Globals.h"

#include <cassert>
#include <array>

// Still 7 free bytes
struct GameState {
	CityState cityState;
	Players players;
	std::mt19937* rng_ptr;
	GameFlags gameFlags;
	Decks decks;
	State currentState;

	void Setup(std::mt19937* externalRng);
	void InfectCitiesSetup();
	void DealPlayerCards();
	void InsertEpidemicCards(std::mt19937* rng, Difficulty diff);
	void SetRoles();

	// -------Actions-------
	inline void DoDrive(uint8_t cityId) {
		players.SetLocation(gameFlags.GetActivePlayer(), cityId);
		DoMedicMovementPassive(gameFlags.GetActivePlayer(), cityId);
	}
	inline void DoDirectFlight(uint8_t cityId) {
		players.SetLocation(gameFlags.GetActivePlayer(), cityId);
		players.RemoveCard(gameFlags.GetActivePlayer(), cityId);
		decks.player_deck.AddToDiscard(cityId);
		DoMedicMovementPassive(gameFlags.GetActivePlayer(), cityId);
	}
	inline void DoCharterFlight(uint8_t cityId) {
		players.SetLocation(gameFlags.GetActivePlayer(), cityId);
		players.RemoveCard(gameFlags.GetActivePlayer(), players.GetLocation(gameFlags.GetActivePlayer()));
		decks.player_deck.AddToDiscard(cityId);
		DoMedicMovementPassive(gameFlags.GetActivePlayer(), cityId);
	}
	inline void DoShuttleFlight(uint8_t cityId) {
		players.SetLocation(gameFlags.GetActivePlayer(), cityId);
		DoMedicMovementPassive(gameFlags.GetActivePlayer(), cityId);
	}

	inline void DoBuild() {
		cityState.AddStation(players.GetLocation(gameFlags.GetActivePlayer()));
		players.RemoveCard(gameFlags.GetActivePlayer(), players.GetLocation(gameFlags.GetActivePlayer()));
		decks.player_deck.AddToDiscard(players.GetLocation(gameFlags.GetActivePlayer()));
	}
	inline void DoTreat(Color color) {
		if (players.roles[gameFlags.GetActivePlayer()] != Role::Medic && !gameFlags.IsCured(color))
			cityState.RemoveDisease(players.GetLocation(gameFlags.GetActivePlayer()), color);
		else
			cityState.RemoveAllDiseases(players.GetLocation(gameFlags.GetActivePlayer()), color);
	}

	inline void DoShare(uint8_t receivingPlayer, uint8_t cardId) {
		players.RemoveCard(gameFlags.GetActivePlayer(), cardId);
		players.AddCard(receivingPlayer, cardId);
	}
	inline void DoDiscover(uint8_t card1, uint8_t card2, uint8_t card3, uint8_t card4, uint8_t card5) {
		players.RemoveCard(gameFlags.GetActivePlayer(), card1);
		decks.player_deck.AddToDiscard(card1);
		players.RemoveCard(gameFlags.GetActivePlayer(), card2);
		decks.player_deck.AddToDiscard(card2);
		players.RemoveCard(gameFlags.GetActivePlayer(), card3);
		decks.player_deck.AddToDiscard(card3);
		players.RemoveCard(gameFlags.GetActivePlayer(), card4);
		decks.player_deck.AddToDiscard(card4);
		players.RemoveCard(gameFlags.GetActivePlayer(), card5);
		decks.player_deck.AddToDiscard(card5);
		gameFlags.SetCured(CardRegistry::GetColor(card1));
	}
	inline void DoDiscover(uint8_t card1, uint8_t card2, uint8_t card3, uint8_t card4) {
		players.RemoveCard(gameFlags.GetActivePlayer(), card1);
		decks.player_deck.AddToDiscard(card1);
		players.RemoveCard(gameFlags.GetActivePlayer(), card2);
		decks.player_deck.AddToDiscard(card2);
		players.RemoveCard(gameFlags.GetActivePlayer(), card3);
		decks.player_deck.AddToDiscard(card3);
		players.RemoveCard(gameFlags.GetActivePlayer(), card4);
		decks.player_deck.AddToDiscard(card4);
		gameFlags.SetCured(CardRegistry::GetColor(card1));
	}

	inline void DoDiscardPlayerCard(uint8_t player_id, uint8_t card_id) {
		players.RemoveCard(player_id, card_id);
		decks.player_deck.AddToDiscard(card_id);
	}

	inline void DoRemoveStation(uint8_t city_id) {
		cityState.RemoveStation(city_id);
	}

	// ----Events----
	inline void DoGovernmentGrant(uint8_t executing_player_id, uint8_t cityId) {
		cityState.AddStation(cityId);
		RemoveEventCard(executing_player_id, (uint8_t)EventCardID::GovGrant);
	}

	inline void DoForecast(uint8_t executing_player_id, uint8_t idx0, uint8_t idx1, uint8_t idx2, uint8_t idx3, uint8_t idx4, uint8_t idx5) {
		decks.infection_deck.ResolveForecast(idx0, idx1, idx2, idx3, idx4, idx5);
		RemoveEventCard(executing_player_id, (uint8_t)EventCardID::Forecast);
	}

	inline void DoResilientPopulation(uint8_t executing_player_id, uint8_t cardId) {
		decks.infection_deck.RemoveFromDiscardPile(cardId);
		RemoveEventCard(executing_player_id, (uint8_t)EventCardID::ResilientPopulation);
	}

	inline void DoOneQuietNight(uint8_t executing_player_id) {
		gameFlags.SetQuietNight(true);
		RemoveEventCard(executing_player_id, (uint8_t)EventCardID::OneQuietNight);
	}

	inline void DoAirlift(uint8_t executing_player_id, uint8_t playerId, uint8_t cityId) {
		players.SetLocation(playerId, cityId);
		DoMedicMovementPassive(playerId, cityId);
		RemoveEventCard(executing_player_id, (uint8_t)EventCardID::Airlift);
	}

	inline void RemoveEventCard(uint8_t executing_player_id, uint8_t event_card_id) {
		// Checking if the event card is used by Contingency Planner
		// If so, the card has to be deleted from the game
		// So we dont put it back into the discard pile

		players.RemoveCard(executing_player_id, event_card_id);

		if (gameFlags.GetContingencyPlannerSlot() != event_card_id) {
			decks.player_deck.AddToDiscard(event_card_id);
			return;
		}

		gameFlags.EmptyContingencyPlannerSlot();
	}

	// ----Roles----
	inline void DoContingencyPlannerTake(uint8_t eventCardId) {
		gameFlags.SetContingencyPlannerSlot(eventCardId);
	}

	inline void DoDispatcher(uint8_t playerId, uint8_t cityId) {
		players.SetLocation(playerId, cityId);
		DoMedicMovementPassive(playerId, cityId);
	}

	inline void DoMedicMovementPassive(uint8_t playerId, uint8_t cityId) {
		if (players.GetRole(playerId) != Role::Medic) return;
		if (gameFlags.IsCured(Color::RED)) cityState.RemoveAllDiseases(cityId, Color::RED);
		if (gameFlags.IsCured(Color::BLUE)) cityState.RemoveAllDiseases(cityId, Color::BLUE);
		if (gameFlags.IsCured(Color::YELLOW)) cityState.RemoveAllDiseases(cityId, Color::YELLOW);
		if (gameFlags.IsCured(Color::BLACK)) cityState.RemoveAllDiseases(cityId, Color::BLACK);
	}

	inline void DoOperationsExpertBuild() {
		cityState.AddStation(players.GetLocation(gameFlags.GetActivePlayer()));
	}

	inline void DoOperationsExpertMovement(uint8_t cityId) {
		players.SetLocation(gameFlags.GetActivePlayer(), cityId);
	}

	// Quarantine Specialist special passive action
	// Medic passive action
	inline bool IsCityProtected(uint8_t cityId, Color color) {
		if (gameFlags.IsGuarantineSpecialistInGame()) {
			// Quarantine Specialist is standing on the city
			if (players.GetLocation(gameFlags.GetQuarantineSpecialistID()) == cityId) return true;
			// Quaratine Specialist is standing in neighbouring city
			if (MapData::IsNeighbor(players.GetLocation(gameFlags.GetQuarantineSpecialistID()), cityId)) return true;
		}
		if (gameFlags.IsMedicInGame()) {
			// Medic also prevents placing disease while the color is cured
			if (gameFlags.IsCured(color) && players.GetLocation(gameFlags.GetMedicID()) == cityId) return true;
		}
		return false;
	}

	inline bool InfectCity(uint8_t city_id, Color color) {
		if (gameFlags.IsEradicated(color)) return false;
		if (IsCityProtected(city_id, color)) return false;

		bool outbreak = cityState.AddDisease(city_id, color);

		if (outbreak && !cityState.HasOutbroken(city_id)) {
			HandleOutbreak(city_id, color);
		}

		return outbreak;
	}

	inline void EndTurn() {
		if (gameFlags.GetActionsRemaining() == 0) {
			// Draw 2 player cards
			DrawTwoPlayerCards();

			// Infect cities
			if (!gameFlags.IsQuietNight()) InfectCities();
			else gameFlags.SetQuietNight(false);

			gameFlags.NextPlayer();
		}
	}

	inline void DrawTwoPlayerCards() {
		for (int i = 0; i < 2; i++) {
			uint8_t card = decks.player_deck.DrawAndRemove();
			if (card == CardRegistry::GetEpidemicCardID()) {
				ResolveEpidemic();
			}
			else {
				players.AddCard(gameFlags.GetActivePlayer(), card);
			}
		}
	}
	inline void InfectCities() {
		for (int i = 0; i < gameFlags.GetInfectionRateAmount(); i++) {
			uint8_t cardId = decks.infection_deck.DrawAndDiscard();
			Color color = CardRegistry::GetColor(cardId);
			InfectCity(cardId, color);
		}
	}

	inline void ResolveEpidemic() {
		/*
		1. Increase: Move the infection rate marker forward 1 space on the
		Infection Rate Track.
		*/
		gameFlags.IncInfectionRate();

		/*
		2. Infect: Draw the bottom card from the Infection Deck. Unless its
		disease color has been eradicated, put 3 disease cubes of that color on
		the named city. If the city already has cubes of this color, do not add
		3 cubes to it. Instead, add just enough cubes so that it has 3 cubes of
		this color and then an outbreak of this disease occurs in the city. 
		Discard this card to the Infection Discard Pile.
		*/
		uint8_t bottom_card = decks.infection_deck.DrawBottomAndDiscard();
		Color card_color = CardRegistry::GetColor(bottom_card);
		if (!gameFlags.IsCured(card_color)) {
			for (int i = 0; i < 3; i++) {
				bool outbreak = InfectCity(bottom_card, card_color);
				if (outbreak) break;
			}
		}

		/*
		3. Intensify: Reshuffle just the cards in the Infection Discard Pile and
		place them on top of the Infection Deck.
		*/
		decks.infection_deck.Intensify(rng_ptr);
	}

	inline void UpdateEradicationFlag() {
		for (int color_id = 0; color_id < Color::COUNT; color_id++) {
			if (cityState.GetTotalCubeCount((Color)color_id) == 0 && gameFlags.IsCured((Color)color_id)) {
				gameFlags.SetEradicated((Color)color_id);
			}
		}
	}

	void GetPossibleActions(ActionList& list) const;
	void AddEventAction(ActionList& list, uint8_t event_card_id, uint8_t card_owner_id) const;
	void AddDispatcherActions(ActionList& list) const;
	void AddOpsExpertActions(ActionList& list) const;

	void Execute(Action action);
	void HandleOutbreak(uint8_t city_id, Color color);
};

#endif