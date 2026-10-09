#pragma once
#define QT_NO_DEPRECATED_WARNINGS

// see Game_Window::show_help() function for game rules and features. at the start of the .cpp file

#include <QApplication>
#include <QWidget>
#include <QStandardItemModel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QListView>
#include <QLineEdit>
#include <QPainter>
#include <QMouseEvent>

import std;
//---------------------------------------------------------------------
struct Room {
	std::array<Room*, 3> tunnel{};
	int index{};
	bool bat_here{ false }, wumpus_here{ false }, pit_here{ false };
};
//---------------------------------------------------------------------
enum class Outcome
{
	not_over,
	fell_into_pit,
	hit_wumpus,
	eaten_by_wumpus,
	ran_out_of_arrows,
	hit_by_ricocheting_arrow
};
//-------------------------------map structure--------------------------------------
class Cave {
public:
	Cave();
	int wumpus_in_room()const;
	void debug_print(std::ostream&) const;

	int player_in_room{};
	int arrows_left{ 5 };
	std::array<Room, 20>rooms;// dodecahedron
};
//---------------------------------------------------------------------
struct Action
{
	enum class Action_type { none, move, shoot, label };
	enum class Action_error { none, no_room_provided, ricocheting_arrow, cant_go_there };
	std::vector<int>rooms_indices;
	Action_type what{ Action_type::none };
	void direct_the_arrow(const Cave& cave, int room_index_a, int room_index_b); // ricocheting arrow
	Action_error validate_action(const Cave&); // on invalid action throws
	Outcome affect(Cave&, std::ostream&);
};
//--------------------------------Game logic------------------------------------- 
class Game
{
public:
	Action run(std::ostream&, std::istream&);
	void debug_print() const;
	const Cave& cave_state()const { return m_cave_state; }
	const Outcome& outcome()const { return m_outcome; }
private:
	Outcome m_outcome{ Outcome::not_over };
	Cave m_cave_state;
};
//--------------------------------a room on the map-------------------------------------
class Index_Circle
{
public:
	Index_Circle(QPoint center, const QString& index, int radius = 27);
	void set_fill_color(QColor c) { m_fill_color = c; }
	void set_checked(bool checked) { m_is_checked = checked; }
	QPoint center() const { return m_center; }
	const QRect& text_box_bounds() const { return m_text_box; }
	int radius() const { return m_radius; }

	void paint(QPainter& painter) const;
private:
	const QPoint m_center;
	const QString m_index;
	const int m_radius;
	const QRect m_text_box;
	bool m_is_checked{ false };
	QColor m_outline_color{ Qt::white };
	QColor m_fill_color{ Qt::black };
};
//---------------------------------------------------------------------
class Cave_Map :public QWidget
{
	Q_OBJECT
public:
	enum Status
	{
		been_here,
		maybe_bat,
		maybe_pit,
		wumpus_slain_here,
		player_slain_here
	};
	explicit Cave_Map(QWidget* parrent = nullptr);
	void mark_on_action(const Game&, Action c);
	void mark_room(int index, Status);
	void check_room(int index) { m_rooms[index].set_checked(true); }
	void uncheck_all_rooms() { for (auto& r : m_rooms)r.set_checked(false); }
signals:
	void room_clicked(int room_index);
protected:
	void paintEvent(QPaintEvent*) override;
	void mousePressEvent(QMouseEvent* event) override;
private:
	// structure
	QPoint point_on_ring(int radius, double angle_degrees) const;
	const QPoint m_center;
	const std::array<int,3> m_connecting_rings_radii;
	std::vector<Index_Circle> m_rooms;
	std::vector<std::pair<int, int>> m_connected_rooms_indiсes;
};
//---------------------------------------------------------------------
class Game_Window :public QWidget
{
public:
	Game_Window();
	void debug_print() const;
private:
	Game m_game; // game data and logic
	//-------------------
	QStandardItemModel m_message_data_model; // contains messages to player
	QListView m_message_view; // draws messages to player
	//------ base 
	QHBoxLayout m_base_layout;
	QVBoxLayout m_in_out_group_layout;
	Cave_Map m_map; // draws map, takes click input
	// --- out_group ---
	QHBoxLayout m_input_layout;
	QPushButton m_help_button;

	QHBoxLayout m_action_buttons_layout;
	QRadioButton m_move;
	QRadioButton m_shoot;
	QRadioButton m_label;

	void update_current_input(std::string new_value) { m_map.uncheck_all_rooms(); update(); m_current_input = new_value; }
	std::string m_current_input;

	QPushButton m_input_button;
	//-------------------
	void on_input(); // game logic
	void update_message(std::ostringstream&);
	void print_on_outcome(std::ostringstream&);
	void show_help();
};
//---------------------------------------------------------------------
