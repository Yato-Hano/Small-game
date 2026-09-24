#define QT_NO_DEPRECATED_WARNINGS
#include <Hunt_the_Wumpus_game.h>

inline int rand_int(int min, int max)
{
	static std::default_random_engine ran{ unsigned(time(0)) };
	return std::uniform_int_distribution<>{min, max}(ran);
}
//------------------------------------------------------------------
void connect_rooms(auto& cave);
void place_wumpus(auto& cave);
void place_bats(auto& cave);
void place_pits(auto& cave);
//------------------------------------------------------------------
Cave::Cave()
	:player_in_room{ rand_int(0,rooms.size() - 1) }
{
	connect_rooms(rooms);
	place_wumpus(rooms);
	place_bats(rooms);
	place_pits(rooms);

	while (rooms[player_in_room].bat_here ||
		rooms[player_in_room].pit_here ||
		rooms[player_in_room].wumpus_here)
	{
		player_in_room = rand_int(0, rooms.size() - 1);
	}
}
int Cave::wumpus_in_room() const
{
	for (const Room& r : rooms)
		if (r.wumpus_here)
			return r.index;
	throw std::string{ "wump not found" };
};
void Cave::debug_print(std::ostream& os) const
{
	os << "player is in the room " << player_in_room << '\n'
		<< "wumpus is in the room " << wumpus_in_room() << '\n';
	for (int i = 0; i < rooms.size(); i++)
	{
		os << "room " << rooms[i].index
			<< " connected to "
			<< rooms[i].tunnel[0]->index << ' '
			<< rooms[i].tunnel[1]->index << ' '
			<< rooms[i].tunnel[2]->index
			<< " wumpus " << rooms[i].wumpus_here
			<< " pit " << rooms[i].pit_here
			<< " bat " << rooms[i].bat_here
			<< '\n';
	}
}
bool bat_is_nearby(const Room& r)
{
	return r.tunnel[0]->bat_here ||
		r.tunnel[1]->bat_here ||
		r.tunnel[2]->bat_here;
}
bool pit_is_nearby(const Room& r)
{
	return r.tunnel[0]->pit_here ||
		r.tunnel[1]->pit_here ||
		r.tunnel[2]->pit_here;
}
bool wumpus_is_nearby(const Room& r)
{
	return r.tunnel[0]->wumpus_here ||
		r.tunnel[1]->wumpus_here ||
		r.tunnel[2]->wumpus_here;
}
std::ostream& operator<<(std::ostream& os, const Cave& gs)
{
	os << "You are in room " << gs.player_in_room << '\n'
		<< "There are paths to rooms ";
	for (int i = 0; i < 3; i++)
		os << gs.rooms[gs.player_in_room].tunnel[i]->index << ' ';
	os << '\n';
	if (wumpus_is_nearby(gs.rooms[gs.player_in_room])) os << "I smell the wumpus\n";
	if (pit_is_nearby(gs.rooms[gs.player_in_room])) os << "I feel a breeze\n";
	if (bat_is_nearby(gs.rooms[gs.player_in_room])) os << "I hear a bat\n";
	os << gs.arrows_left << " arrows left.\n";
	return os;
}
//------------------------------------------------------------------
bool is_connected(const Room& r1, const Room& r2)
{
	return r1.tunnel[0] == &r2 ||
		r1.tunnel[1] == &r2 ||
		r1.tunnel[2] == &r2;
}
Room* rand_adj_room(const Room* r)
{
	return r->tunnel[rand_int(0, 2)];
}

void Action::direct_the_arrow(const Cave& cave,int room_index_a,int room_index_b)
{
	rooms_indices[room_index_a] = rand_adj_room(&cave.rooms[rooms_indices[room_index_a]])->index;
	rooms_indices.erase(rooms_indices.begin() + room_index_b, rooms_indices.end());
}

Action::Action_error Action::validate_action(const Cave& cave)
{
	if (rooms_indices.empty()) return Action_error::no_room_provided; // throw std::string{ "Provide a room number" };

	switch (what)
	{
	case Action_type::shoot:

		if (!is_connected(cave.rooms[cave.player_in_room], cave.rooms[rooms_indices[0]]))
			//ricocheting arrow (can be player's room)
		{
			direct_the_arrow(cave,0,1);
			return Action_error::ricocheting_arrow;
		}
		for (int i = 1; i < rooms_indices.size(); i++)
		{
			if (!is_connected(cave.rooms[rooms_indices[i]], cave.rooms[rooms_indices[i - 1]]))
				//ricocheting arrow (can be player's room)
			{
				direct_the_arrow(cave,i-1,i);
				return Action_error::ricocheting_arrow;
			}
		}
		break;
	case Action_type::move:
		if (!is_connected(cave.rooms[cave.player_in_room], cave.rooms[rooms_indices[0]]))
		{
			return Action_error::cant_go_there;
		}
		break;
	}
	return Action_error::none;
}
Outcome Action::affect(Cave& cave, std::ostream& os)
{
	if (what == Action::Action_type::label) return Outcome::not_over;
	qDebug()<<std::format("rooms provided{}", rooms_indices);

	switch (Action_error action_error{ validate_action(cave) })
	{
	case Action_error::no_room_provided:
		throw std::string{ "Provide a room index.\n" };
		break;
	case Action_error::cant_go_there:
		throw std::string{ "Cant go there, the rooms are not connected.\n" };
		break;
	case Action_error::ricocheting_arrow:
		os << "Ricocheting arrow. Rooms were not connected.\n";
		break;
	}
	qDebug() << std::format("rooms result{}", rooms_indices);

	switch (what)
	{
	case Action_type::move:
		cave.player_in_room = rooms_indices[0];
		while (cave.rooms[cave.player_in_room].bat_here)
		{
			os << "You've been caught by a giant bat.\n";
			Room* new_room = rand_adj_room(&cave.rooms[cave.player_in_room]);
			cave.player_in_room = new_room->index;
		}
		break;
	case Action_type::shoot:
	{
		for (const int& n : rooms_indices)
		{
			if (cave.player_in_room==n)
				return Outcome::hit_by_ricocheting_arrow;

			if (cave.rooms[n].wumpus_here)
				return Outcome::hit_wumpus;
		}
		os << "You woke up the wumpus and it moved to another room.\n";
		--cave.arrows_left;
		Room* wump_room = &cave.rooms[cave.wumpus_in_room()];
		wump_room->wumpus_here = false;
		Room* new_room = rand_adj_room(wump_room);
		new_room->wumpus_here = true;
		break;
	}
	case Action_type::label:
		break;
	default:
		throw std::string{ "affect() invalid action" };
	}

	if (cave.rooms[cave.player_in_room].wumpus_here)
		return Outcome::eaten_by_wumpus;
	if (cave.rooms[cave.player_in_room].pit_here)
		return Outcome::fell_into_pit;
	if (cave.arrows_left == 0)
		return Outcome::ran_out_of_arrows;

	return Outcome::not_over;
}
/*
examples:
s13 4 3
m13
l13
*/
std::istream& operator>>(std::istream& is, Action& a)
{
	a = {}; // clear
	char ch{};
	is >> ch;
	Action::Action_type at{ Action::Action_type::none };
	switch (ch)
	{
	case 'm':
		at = Action::Action_type::move;
		break;
	case 's':
		at = Action::Action_type::shoot;
		break;
	case 'l':
		at = Action::Action_type::label;
		break;
	default:
		throw std::string{ "invalid action (use m or s)" };

	}
	int room_n{ 0 };
	std::vector<int>r;
	for (int i = 0; i < 3 && is >> room_n; i++)
	{
		if (room_n < 0 || 19 < room_n)
			throw std::string{ "invalid room number" };

		r.push_back(room_n);
		
		if (char space{  }; is.get(space) && !iswspace(space)) 
			break;
	}
	// if (r.empty()) throw std::string{ "provide a room number" }; 
	a.rooms_indices = std::move(r);
	a.what = at;
	return is;
}
//------------------------------------------------------------------
void Game::debug_print() const
{
	std::ostringstream os;
	m_cave_state.debug_print(os);
	std::istringstream is{ os.str() };
	for (std::string line; std::getline(is, line);)
	{
		qDebug() << line.c_str();
	}
}
void print_outcome(std::ostream& os, Outcome result)
{
	switch (result)
	{
	case Outcome::fell_into_pit:
		os << "You fell into bottomless pit.\n";
		break;
	case Outcome::hit_wumpus:
		os << "You hit the Wumpus.\n";
		break;
	case Outcome::eaten_by_wumpus:
		os << "You were eaten by the Wumpus.\n";
		break;
	case Outcome::ran_out_of_arrows:
		os << "You ran out of arrows.\n";
		break;
	case Outcome::hit_by_ricocheting_arrow:
		os << "You were hit by your own ricocheting arrow.\n";
		break;
	}
}
Action Game::run(std::ostream& os, std::istream& is)
{
	Action action;
	try
	{
		is >> action;
		m_outcome = action.affect(m_cave_state, os);
	}
	catch (std::string& err_message)
	{
		os << err_message << '.' << '\n';
	}
	return action;
}
//----------------------------------------------------------------
void connect_rooms(auto& cave)
{
	cave[0].tunnel = { &cave[1],&cave[4],&cave[7] };
	cave[1].tunnel = { &cave[0],&cave[2],&cave[9] };
	cave[2].tunnel = { &cave[1],&cave[3],&cave[11] };
	cave[3].tunnel = { &cave[2],&cave[4],&cave[13] };
	cave[4].tunnel = { &cave[0],&cave[3],&cave[5] };
	cave[5].tunnel = { &cave[4],&cave[6],&cave[14] };
	cave[6].tunnel = { &cave[5],&cave[7],&cave[16] };
	cave[7].tunnel = { &cave[0],&cave[6],&cave[8] };
	cave[8].tunnel = { &cave[7],&cave[9],&cave[17] };
	cave[9].tunnel = { &cave[1],&cave[8],&cave[10] };
	cave[10].tunnel = { &cave[9],&cave[11],&cave[18] };
	cave[11].tunnel = { &cave[2],&cave[10],&cave[12] };
	cave[12].tunnel = { &cave[11],&cave[13],&cave[19] };
	cave[13].tunnel = { &cave[3],&cave[12],&cave[14] };
	cave[14].tunnel = { &cave[5],&cave[13],&cave[15] };
	cave[15].tunnel = { &cave[14],&cave[16],&cave[19] };
	cave[16].tunnel = { &cave[6],&cave[15],&cave[17] };
	cave[17].tunnel = { &cave[8],&cave[16],&cave[18] };
	cave[18].tunnel = { &cave[10],&cave[17],&cave[19] };
	cave[19].tunnel = { &cave[12],&cave[15],&cave[18] };

	for (int i = 0; i < cave.size(); i++)
		cave[i].index = i;
}
void place_wumpus(auto& cave)
{
	int w = rand_int(0, cave.size() - 1);
	cave[w].wumpus_here = true;
}
void place_bats(auto& cave)
{
	int w = rand_int(0, cave.size() - 1);
	cave[w].bat_here = true;
	w = rand_int(0, cave.size() - 1);
	cave[w].bat_here = true;
}
void place_pits(auto& cave)
{
	int w = rand_int(0, cave.size() - 1);
	cave[w].pit_here = true;
	w = rand_int(0, cave.size() - 1);
	cave[w].pit_here = true;
}
//----------------------------------------------------------------
Index_Circle::Index_Circle(QPoint center, const QString& index, int radius)
	:m_center{ center },
	m_index{ index },
	m_radius{ radius },
	m_text_box{ QPoint{center.x() - radius ,center.y() - radius},QSize{radius * 2,radius * 2} }
{
}
void Index_Circle::paint(QPainter& painter) const
{
	painter.save();
	
	//drawing circle
	painter.setBrush(m_fill_color);
	painter.setPen(Qt::NoPen);
	painter.drawEllipse(m_center, m_radius, m_radius);

	//drawing text
	painter.setPen(Qt::white);
	painter.drawText(text_box_bounds(),Qt::AlignCenter, m_index);

	painter.restore();
}
//----------------------------------------------------------------

Cave_Map::Cave_Map(QWidget* parrent)
	:QWidget{ parrent },
	m_center{ 300,300 }, //700,300
	m_connecting_rings_radii{ 225, 150, 75 }	
{
	setMinimumSize(600, 600);

	double angle = -90.0;
	double increment = 360.0 / 5;

	m_rooms.reserve(20);
	for (int i = 0; i < 5; i++) {
		m_rooms.emplace_back(point_on_ring(m_connecting_rings_radii[0], angle), QString::number(i));
		if (i < 4) angle += increment; // to start the next one at the same angle on the inner ring
	}

	increment = 360.0 / 10;
	for (int i = 5; i < 15; i++) {
		m_rooms.emplace_back(point_on_ring(m_connecting_rings_radii[1], angle), QString::number(i));
		if (i < 14) angle += increment;  // to start the next one at the same angle on the inner ring
	}

	increment = 360.0 / 5;
	for (int i = 15; i < 20; i++, angle += increment) {
		m_rooms.emplace_back(point_on_ring(m_connecting_rings_radii[2], angle), QString::number(i));
	}

	m_connected_rooms_indiсes.emplace_back( 0, 7 );
	m_connected_rooms_indiсes.emplace_back( 1, 9 );
	m_connected_rooms_indiсes.emplace_back( 4, 5 );
	m_connected_rooms_indiсes.emplace_back( 3, 13 );
	m_connected_rooms_indiсes.emplace_back( 11, 2 );
	m_connected_rooms_indiсes.emplace_back( 6, 16 );
	m_connected_rooms_indiсes.emplace_back( 14, 15 );
	m_connected_rooms_indiсes.emplace_back( 8, 17 );
	m_connected_rooms_indiсes.emplace_back( 10, 18 );
	m_connected_rooms_indiсes.emplace_back( 19, 12 );
}
void Cave_Map::mark_on_action(const Game& g, Action action)
{
	mark_room(g.cave_state().player_in_room, Status::been_here);
	if (action.what == Action::Action_type::label)
	{
		mark_room(action.rooms_indices[0], Status::maybe_pit);
	}
}
void Cave_Map::mark_room(int index, Status status)
{
	if (index < 0 || index >= m_rooms.size())
		throw std::out_of_range("invalid room index");

	switch (status) {
	case Status::been_here:
		m_rooms[index].set_fill_color(Qt::cyan);
		break;
	case Status::maybe_pit:
		m_rooms[index].set_fill_color(QColor(139, 0, 0));
		break;
	default:
		throw std::runtime_error("invalid room status");
	}

	update();
}
void Cave_Map::paintEvent(QPaintEvent* /**/)
{
	QPen pen{ QColor{ 0,100,0 } };
	pen.setWidth(3);

	QPainter painter{ this };
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setPen(pen);
	painter.setBrush(Qt::NoBrush);

	for (const int& radius : m_connecting_rings_radii)
	{
		painter.drawEllipse(m_center, radius, radius);
	}
	for (const auto& [a, b] : m_connected_rooms_indiсes)
	{
		painter.drawLine(m_rooms[a].center(), m_rooms[b].center());
	}
	for (const Index_Circle& room : m_rooms)
	{
		room.paint(painter);
	}
}
QPoint Cave_Map::point_on_ring(int radius, double angle_degrees) const
{
	double radians = angle_degrees * std::numbers::pi / 180.0;
	int x = m_center.x() + radius * std::cos(radians);
	int y = m_center.y() + radius * std::sin(radians);
	return { x, y };
}
void Cave_Map::mousePressEvent(QMouseEvent* event)
{
	for (int i = 0; i < m_rooms.size(); i++)
	{
		if (QRegion circle_region{ m_rooms[i].text_box_bounds(), QRegion::Ellipse };
			circle_region.contains(event->pos()))
		{
			emit room_clicked(i);
			return;
		}
	}
}
//----------------------------------------------------------------
Game_Window::Game_Window()
	:message_data_model{ this },
	base_layout{ this },
	in_out_group{ this },
	map{ this },
	in_out_group_layout{ &in_out_group },
	message_view{ &in_out_group },
	input_group{ &in_out_group },
	input_layout{ &input_group },
	help_button{ "Help", &input_group },
	input_field{ &input_group },
	input_button{"Input", &input_group}
{
	setWindowIcon(QIcon{ "images/cave.png" });

	resize(1200, 600);

	base_layout.addWidget(&in_out_group);
	base_layout.addWidget(&map);

	in_out_group_layout.addWidget(&message_view);
	in_out_group_layout.addWidget(&input_group);

	input_layout.addWidget(&help_button);
	input_layout.addWidget(&input_field);
	input_layout.addWidget(&input_button);

	message_view.setModel(&message_data_model);

	input_field.setPlaceholderText("move, shoot, label (m, s, l)");

	connect(&help_button, &QPushButton::clicked, this, &Game_Window::show_help);
	connect(&input_field, &QLineEdit::returnPressed, this, &Game_Window::on_input);
	connect(&input_button, &QPushButton::clicked, this, &Game_Window::on_input);
	connect(&map, &Cave_Map::room_clicked, this, 
		[this](int room_index)
		{
			input_field.insert(' ' + QString::number(room_index));
		});

	std::ostringstream os;
	os << game.cave_state();
	update_message(os);
	map.mark_room(game.cave_state().player_in_room, Cave_Map::Status::been_here);
}
void Game_Window::show_help()
{
	QMessageBox::information(this,"How to play",
		"Your job is to slay the wumpus using bow and arrow.\n"
		"In addition to the wumpus, the cave has two hazards: bottomless pits and giant bats.\n"
		"If you enter a room with a bottomless pit, it’s the end of the game for you.\n"
		"If you enter a room with a bat, the bat picks you up and drops you into another room.\n"
		"If you enter the room with the wumpus or he enters yours, he eats you.\n"
		"You can click the map instead of typing room numbers.\n"
		"Example: s13 3 4 shoots through rooms 13, then 3, then 4.\n"
		"If rooms are not connected, arrow will ricochet in a random adjacent room, it may be your room.\n"
	);
}
void Game_Window::update_message(std::ostringstream& os)
{
	message_data_model.clear();
	print_outcome(os, game.outcome()); // can add an end game message
	std::istringstream is{ os.str() };
	for (std::string line; std::getline(is, line);)
	{
		message_data_model.appendRow(new QStandardItem{ QString{line.c_str()}});
	}
}
void Game_Window::on_input()
{
	std::ostringstream os;
	if (game.outcome() == Outcome::not_over)
	{
		std::istringstream is{ input_field.text().toUtf8().constData() };
		input_field.clear();

		Action action = game.run(os, is);
		map.mark_on_action(game, action);

		if (game.outcome() == Outcome::not_over)
		{
			os << game.cave_state() << '\n';
		}
	}
	update_message(os);
}
void Game_Window::debug_print() const
{
	game.debug_print();
}
//-----------------------------------------------------
int main(int argc, char* argv[])
try
{
	QApplication app{ argc, argv };

	app.setFont(QFont{ "Segoe UI",14 });

	Game_Window game_window;
	game_window.show();
	//game_window.debug_print();

	return app.exec();
}
catch (const std::runtime_error& surprise)
{
	std::ofstream ofs{ "ERROR_runtime_error.txt" };
	ofs << surprise.what();
	return 1;
}
catch (const std::exception& surprise)
{
	std::ofstream ofs{ "ERROR_exception.txt" };
	ofs << surprise.what();
	return 2;
}
catch (...)
{
	std::ofstream ofs{ "ERROR_unknown_exception.txt" };
	ofs << "Caught an unknown exception.";
	return 3;
}
//-----------------------------------------------------