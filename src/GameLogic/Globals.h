#ifndef GLOBALS_H
#define GLOBALS_H

#include <iostream>
#include <string>
#include <array>

const int NUMBER_OF_ROLE_CARDS = 7;
const int NUMBER_OF_CITY_CARDS = 48;
const int NUMBER_OF_INFECTION_CARDS = 48;
const int NUMBER_OF_EPIDEMIC_CARDS = 6;
const int NUMBER_OF_EVENT_CARDS = 5;
const int NUMBER_OF_CITIES_PER_COLOR = 12;
const int NUMBER_OF_COLORS = 4;
const int NUMBER_OF_DISEASE_CUBES_PER_COLOR = 24;
const int OUTBREAK_MARKER_MAX = 8;
const int MAX_RESEARCH_LAB_COUNT = 6;
const int MAX_INFECTION_RATE_MARKER_VALUE = 7;
const int MAX_DISEASE_SCORE = 3;
const int NUMBER_OF_CITIES = 48;
const int HAND_LIMIT = 7;
const int NUMBER_OF_UNIQUE_CARDS = NUMBER_OF_CITIES + NUMBER_OF_EVENT_CARDS;
const int NUMBER_OF_MAX_PLAYERS = 4;

const int INFECTION_DECK_SIZE = NUMBER_OF_CITIES;
const int PLAYER_DECK_SIZE = NUMBER_OF_CITIES + NUMBER_OF_EVENT_CARDS;
const int MIN_EPIDEMIC_CARD = 4;

enum Color : uint8_t {
    BLUE = 0,
    YELLOW = 1,
    BLACK = 2,
    RED = 3,
    COUNT = 4,
	NO_COLOR = 5
};

enum Difficulty : uint8_t {
	INTRO = 0,
	STANDART = 1,
	HEROIC = 2
};

enum Role : uint8_t {
	Contingency = 0,
	Dispatcher = 1,
	Medic = 2,
	Operations = 3,
	Quarantine = 4,
	Researcher = 5,
	Scientist = 6
};

enum EventCardID : uint8_t {
	Airlift = 48,
	GovGrant = 49,
	Forecast = 50,
	OneQuietNight = 51,
	ResilientPopulation = 52
};

enum State : uint8_t {
	InProgress = 0,
	AllCured = 1,
	OutbreakMarkerMaxed = 2,
	NoMoreDiseaseCubes = 3,
	NotEnoughPlayerCards = 4
};

const std::array<std::string, NUMBER_OF_CITIES_PER_COLOR> BLUE_CITIES = {
	"Atlanta",
	"San Francisco",
	"Chicago",
	"Montreal",
	"New York",
	"Washington",
	"London",
	"Madrid",
	"Paris",
	"Essen",
	"Milan",
	"Saint Petersburg"
};

const std::array<std::string, NUMBER_OF_CITIES_PER_COLOR> YELLOW_CITIES = {
	"Los Angeles",
	"Mexico City",
	"Miami",
	"Bogota",
	"Lima",
	"Santiago",
	"Buenos Aires",
	"Sao Paulo",
	"Lagos",
	"Khartoum",
	"Kinshasa",
	"Johannesburg"
};

const std::array<std::string, NUMBER_OF_CITIES_PER_COLOR> BLACK_CITIES = {
	"Algiers",
	"Cairo",
	"Istanbul",
	"Riyadh",
	"Moscow",
	"Baghdad",
	"Tehran",
	"Karachi",
	"Delhi",
	"Mumbai",
	"Kolkata",
	"Chennai"
};

const std::array<std::string, NUMBER_OF_CITIES_PER_COLOR> RED_CITIES = {
	"Beijing",
	"Shanghai",
	"Seoul",
	"Tokyo",
	"Osaka",
	"Taipei",
	"Hong Kong",
	"Bangkok",
	"Jakarta",
	"Manila",
	"Sydney",
	"Ho Chi Minh City"
};

#endif