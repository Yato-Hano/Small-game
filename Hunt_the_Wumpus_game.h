#pragma once
#define QT_NO_DEPRECATED_WARNINGS
/*
		About the game:
		"Your job is to slay the wumpus using bow and arrow.\n"
		"In addition to the wumpus, the cave has two hazards: bottomless pits and giant bats.\n"
		"If you enter a room with a bottomless pit, it’s the end of the game for you.\n"
		"If you enter a room with a bat, the bat picks you up and drops you into another room.\n"
		"If you enter the room with the wumpus or he enters yours, he eats you.\n"
		"You can click the map instead of typing room numbers.\n"
		"Example: s13 3 4 shoots through rooms 13, then 3, then 4.\n"
		"If rooms are not connected, arrow will ricochet in a random adjacent room, it may be your room.\n"
*/
#include <array>
#include <numbers>
#include <random>
#include <iostream>
#include <format>
#include <string>
#include <fstream>

#include <QApplication>
#include <QWidget>
#include <QStandardItemModel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QListView>
#include <QLineEdit>
#include <QPainter>
#include <QMouseEvent>

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
//---------------------------------------------------------------------
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
//---------------------------------------------------------------------
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
//---------------------------------------------------------------------
class Index_Circle
{
public:
	Index_Circle(QPoint center, const QString& index, int radius = 27);
	void set_fill_color(QColor c) { m_fill_color = c; }
	QPoint center() const { return m_center; }
	const QRect& text_box_bounds() const { return m_text_box; }
	int radius() const { return m_radius; }
	void paint(QPainter& painter) const;
private:
	const QPoint m_center;
	const QString m_index;
	const int m_radius;
	const QRect m_text_box;
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
		maybe_pit
	};
	explicit Cave_Map(QWidget* parrent = nullptr);
	void mark_on_action(const Game&, Action c);
	void mark_room(int index, Status);
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
	Game game; // game data and logic
	//-------------------
	QStandardItemModel message_data_model;
	//------
	QHBoxLayout base_layout;
	QWidget in_out_group;
	Cave_Map map; // draws map
	// --- in_out_group ---
	QVBoxLayout in_out_group_layout;
	QListView message_view;
	// --- input_group ---
	QWidget input_group;
	QHBoxLayout input_layout;
	QPushButton help_button;
	QLineEdit input_field;
	QPushButton input_button;
	//-------------------
	void on_input(); // game logic
	void update_message(std::ostringstream&);
	void show_help();
};
//---------------------------------------------------------------------
