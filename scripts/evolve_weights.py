import optuna
import pandemic_cpp
import random

def objective(trial):
    weights = pandemic_cpp.Weights()

    weights.cure_weight = trial.suggest_float("cure_weight", 0.0, 20.0)
    weights.card_progression = trial.suggest_float("card_progression", 0.0, 10.0)
    weights.station_dist_reward = trial.suggest_float("station_dist_reward", 0.0, 10.0)
    weights.outbreak_penalty = trial.suggest_float("outbreak_penalty", 0.0, 10.0)
    weights.hotspot_penalty = trial.suggest_float("hotspot_penalty", 0.0, 10.0)
    weights.cube_pressure = trial.suggest_float("cube_pressure", 0.0, 10.0)
    weights.deck_progress_penalty = trial.suggest_float("deck_progress_penalty", 0.0, 10.0)
    weights.hotspot_approach_weight = trial.suggest_float("hotspot_approach_weight", 0.0, 10.0)
    weights.station_network_weight = trial.suggest_float("station_network_weight", 0.0, 10.0)
    weights.chain_reaction_penalty = trial.suggest_float("chain_reaction_penalty", 0.0, 10.0)
    weights.rendezvous_penalty_weight = trial.suggest_float("rendezvous_penalty_weight", 0.0, 10.0)
    weights.researcher_meetup_weight = trial.suggest_float("researcher_meetup_weight", 0.0, 10.0)
    weights.medic_treat_weight = trial.suggest_float("medic_treat_weight", 0.0, 10.0)
    weights.qs_protect_weight = trial.suggest_float("qs_protect_weight", 0.0, 10.0)

    total_score = 0.0
    games = 20

    for i in range(games):
        seed = random.randrange(0, 1_000_000)
        env = pandemic_cpp.PandemicEnv(0, 4, seed)
        env.weights = weights

        result = env.play_macro_game(1000)

        if result["won"]:
            total_score += 1000.0
        else:
            status = result["status"]
            if status == "Loss_Outbreaks":
                total_score -= 1000.0
            elif status == "Loss_CubesEmpty":
                total_score -= 1000.0
            elif status == "Loss_DeckEmpty":
                total_score -= 1000.0
            else:
                total_score -= 1000.0

            total_score += float(result["cured_count"]) * 250.0

    return total_score / games


study = optuna.create_study(direction="maximize")
study.optimize(objective, n_trials=1000, n_jobs=15)

print("Best Weights:", study.best_params)