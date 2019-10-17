#include "States/GameSimulator.hpp"

namespace cp {
	GameSimulator::GameSimulator(GameDataRef res_store)
		: resource_store(res_store), map(res_store), pool(500),bar(res_store) {
	}
	GameSimulator::~GameSimulator() {
	}
	void GameSimulator::init() {
		fout<<"Executing init"<<std::endl;
		fout << "Assigning id to each bot" << std::endl;
		init_car_res();
		resource_store->assets.load_texture("Bullet", "../res/bullet.png");
		fout << "Car Assests Loaded" <<std::endl;
		for (int i = 0; i < TOTAL_BOTS; i++) {
			add_bot_players();
		}
		// players_map.at(main_player_id).e_position.x = 0;
		// players_map.at(main_player_id).e_position.z = 10000;
		bot_inp_reg();
		map.init();

		resource_store->assets.load_texture("GameOverState background", GAME_OVER_BACKGROUND_FILEPATH);
		font = resource_store->assets.get_font("sfafont");

		///////////////////////////////////////////////
		for (int i = 0; i < 5; i++)
		{
			text[i].setFont(font);
			text[i].setCharacterSize(36);
			text[i].setStyle(sf::Text::Bold);
			text[i].setOutlineColor(sf::Color::White);
			text[i].setOutlineThickness(2);
			text[i].setFillColor(sf::Color::Red);
		}
		text[0].setString("Puntaje:");
		text[0].setPosition(SCREEN_WIDTH / 100, SCREEN_HEIGHT / 100);
		text[1].setPosition(SCREEN_WIDTH / 100, SCREEN_HEIGHT / 100 + 30);
		/*text[2].setString("Health:");
		text[2].setPosition(SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 100);*/
		bar.init(sf::Vector2f(300, 20), sf::Vector2f(SCREEN_WIDTH / 2 - 150, 70), sf::Color::White, sf::Color::Black);

	}
	void GameSimulator::handle_input(float delta) {
		sf::Event event;
		while (resource_store->window.pollEvent(event)) {
			if (sf::Event::Closed == event.type) {
				resource_store->machine.add_state(StateRef(new GameOverState(resource_store)), true);
			}
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q)) {
				std::cout << "Exiting ClientState" << std::endl;
				resource_store->machine.add_state(StateRef(new MainMenuState(resource_store)), true);
			}
			if(sf::Keyboard::isKeyPressed(sf::Keyboard::U)) {
				generate_log();
			}
		}


		AI_bot_output();


		update_controllable(delta);
		//Score done.
		if (players_map.at(main_player_id).e_speed.z > 0)
			score += delta * (players_map.at(main_player_id).e_speed.z);

		map.bound_entity(main_camera);
		for (auto &player_i : players_map) {
			map.bound_entity(player_i.second);
		}

		for(int i=0; i<bullet_set[current].size(); i++) {
			async_update[i].get();
		}
		std::vector<Car *> entities = get_entity_list();
		// Collision working
		Collision::simulate_physics(entities, map);
		for(auto &b: players_map){
			if (b.first == main_player_id)continue;
				for (auto itr : bullet_set[current])
				{
					if (itr->e_position.z - players_map.at(main_player_id).e_position.z > 10000)
						itr->in_use = false;
					collision.handle_collision(*itr, b.second, map, 0);
					if (itr->in_use)
						bullet_set[!current].insert(itr);
					else
						pool.returnObject(itr);
				}
			bullet_set[current].clear();
			current = !current;
		}

		bar.percentage = (players_map.at(main_player_id).health);
		// std::cout << "CAR health" << players_map.at(main_player_id).health << std::endl;
		//text[0].setString("Puntaje: " + std::to_string(players_map.at(main_player_id).e_speed.z));//std::to_string(score/300));
		text[0].setString("Puntaje: " + std::to_string(score/300));
	}
	void GameSimulator::draw(float delta) {
		resource_store->window.clear(sf::Color(105, 205, 4));
		if(!is_main_player_available())return;

		map.draw(500, main_camera);

		draw_all_players();

		////// Finding camera position and camera height ///
		int startPos = map.get_grid_index(main_camera.getPosition().z);
		///////////////////////////////////////////////////
		for (int n = startPos + 500; n > startPos; n--) {
			//drawSprite(map.lines[n % map.getGridCount()]);
			for(auto &player_i:players_map) {
				int index = map.get_grid_index(player_i.second.getPosition().z) ;
				if (index >= n - 1 and index <= n + 1){
					//draw_player(player_i.second);
				}
			}
		}

		for (auto itr : bullet_set[current])
		{
			int index = (itr->e_position.z) / map.getSegL();
			//itr->drawSprite(map.lines[index]);
		}
		//bar.draw();
		for (int i = 0; i < 2; i++)
		{
			resource_store->window.draw(text[i]);
		}

		resource_store->window.display();
	}
	void GameSimulator::update(float delta) {
		focus_on(main_player_id);
		map.bound_entity(main_camera);
		for(auto &player_i:players_map) {
			map.bound_entity(player_i.second);
		}
		for (auto itr : bullet_set[current])
			map.bound_entity(*itr);
		for (auto itr : bullet_set[current])
			itr->update_car(delta, map.lines, map.getSegL());
		std::vector<Car*> entities = get_entity_list();
		// Collision working
		Collision::simulate_physics(entities, map);
		bar.percentage = players_map.at(main_player_id).health;
		if(players_map.at(main_player_id).health==0){
			resource_store->machine.add_state(StateRef(new BustedState(resource_store)), true);
		}
		// std::cout << "CAR health" << players_map.at(main_player_id).health << std::endl;
		if (players_map.at(main_player_id).e_speed.z>200.0 && ((score/300)%5)==1){
			if(bot_players_count<10){
				add_bot_players();
			}
		}
		text[0].setString("Velocidad: " + std::to_string(players_map.at(main_player_id).e_speed.z));//std::to_string(score/300));
		text[1].setString("Puntaje: " + std::to_string(score/300));
		//text[0].setString("Puntaje: " + std::to_string((int)score/300));
	}
	std::vector<Car*> GameSimulator::get_entity_list() {
		std::vector<Car*> entities;
		for(auto &player:players_map)entities.push_back(&player.second);
		return entities;
	}
	void GameSimulator::drawSprite(Line &line) {
		sf::Sprite s = line.sprite;
		int w = s.getTextureRect().width;
		int h = s.getTextureRect().height;

		float destX = line.X + line.scale * line.spriteX * map.getScreenWidth() / 2;
		float destY = line.Y + 4;
		float destW = w * line.W / 266;
		float destH = h * line.W / 266;

		destX += destW * line.spriteX; //offsetX
		destY += destH * (-1);		   //offsetY

		float clipH = destY + destH - line.clip;
		if (clipH < 0)
			clipH = 0;
		if (clipH >= destH)
			return;
		s.setTextureRect(sf::IntRect(0, 0, w, h - h * clipH / destH));
		s.setScale(destW / w, destH / h);
		s.setPosition(destX, destY);
		resource_store->window.draw(s);
	}
	GameSimulatorSnap GameSimulator::get_current_snap(SnapFlag flag) {
		if(flag == SnapFlag::NETWORK_SNAP) {
			return GameSimulatorSnap(
				ext_players_count, bot_players_count, MAX_EXT_ALLOWED, main_player_id, players_map);
		}
		if(flag == SnapFlag::OFFLINE_SNAP) {
			return GameSimulatorSnap(
				ext_players_count, bot_players_count, MAX_EXT_ALLOWED, main_player_id, players_map);
		}
	}
	GameSimulationLog GameSimulator::use_snap(const GameSimulatorSnap &snap, bool is_forced) {
		ext_players_count = snap.ext_players_count;
		bot_players_count = snap.bot_players_count;
		players_map.clear();
		for (auto &player_i : snap.data) {
			players_map.insert(std::pair<ID, PlayerCar>(player_i.first, generate_bot(player_i.second)));
		}
	}
	PlayerCar GameSimulator::generate_bot(const entity_info &info) {
		PlayerCar car(resource_store, 5);
		car.e_position.x = info.x;
		car.e_position.y = info.y;
		car.e_position.z = info.z;
		return car;
	}
}
