#include "tool.h"

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

#define TOOL_MEMBERS /*someone please replace this with something sane*/                                                                                       \
	X(path)                                                                                                                                                    \
	X(arguments)                                                                                                                                               \
	X(input)                                                                                                                                                   \
	X(output)                                                                                                                                                  \
	X(error)                                                                                                                                                   \
	X(activation)                                                                                                                                              \
	X(working_directory)                                                                                                                                       \
	X(timeout)

static void write(const QString &data, const QString &name, QJsonObject &json) {
	json[name] = data;
}
static void write(const Tool_output_target::Type &data, const QString &name, QJsonObject &json) {
	json[name] = data;
}
static void write(const Tool::Activation &data, const QString &name, QJsonObject &json) {
	json[name] = data.toString();
}
static void write(const std::chrono::milliseconds &data, const QString &name, QJsonObject &json) {
	json[name] = qint64{data.count()};
}

static void read(QString &data, const QString &name, QJsonObject &json) {
	data = json[name].toString();
}
static void read(Tool_output_target::Type &data, const QString &name, QJsonObject &json) {
	data = static_cast<Tool_output_target::Type>(json[name].toInt());
}
static void read(Tool::Activation &data, const QString &name, QJsonObject &json) {
	data = json[name].toString();
}
static void read(std::chrono::milliseconds &data, const QString &name, QJsonObject &json) {
	data = std::chrono::milliseconds{static_cast<long>(json[name].toDouble())};
}

QString Tool::to_string() const {
	QJsonObject object;
#define X(Y) write(Y, #Y, object);
	TOOL_MEMBERS
#undef X
	QJsonDocument json{object};
	return QString::fromUtf8(json.toJson());
}

Tool Tool::from_string(const QString &data) {
	Tool tool;
	auto json = QJsonDocument::fromJson(data.toUtf8()).object();
#define X(Y) read(tool.Y, #Y, json);
	TOOL_MEMBERS
#undef X
	return tool;
}

QString Tool::get_name() const {
	return path.split('/').last();
}

//takes an `std::tuple` and returns an `std::tuple` without the last element
template <class Tuple, std::size_t... indexes>
static constexpr auto remove_last_tuple_element(Tuple tuple, std::index_sequence<indexes...>) {
	return std::make_tuple(std::move(std::get<indexes>(tuple))...);
}
template <class Tuple>
static constexpr auto remove_last_tuple_element(Tuple &&tuple) {
	return remove_last_tuple_element(std::forward<Tuple>(tuple), std::make_index_sequence<std::tuple_size_v<Tuple> - 1>());
}

static constexpr auto get_members() { //this may some day be replacable with reflection
#define X(Y) &Tool::Y,
	return remove_last_tuple_element(std::make_tuple(TOOL_MEMBERS 0));
#undef X
}

template <class Comparator, std::size_t... indexes>
static bool compare(const Tool &lhs, const Tool &rhs, std::index_sequence<indexes...>) {
	constexpr auto members = get_members();
	return Comparator{}(std::tie(lhs.*std::get<indexes>(members)...), std::tie(rhs.*std::get<indexes>(members)...));
}

bool operator==(const Tool &lhs, const Tool &rhs) {
	return compare<std::equal_to<>>(lhs, rhs, std::make_index_sequence<std::tuple_size_v<decltype(get_members())>>());
}

bool operator<(const Tool &lhs, const Tool &rhs) {
	return compare<std::less<>>(lhs, rhs, std::make_index_sequence<std::tuple_size_v<decltype(get_members())>>());
}
