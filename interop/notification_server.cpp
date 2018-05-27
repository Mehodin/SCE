#include "notification_server.h"

#include <algorithm>
#include <boost/asio/buffer.hpp>
#include <boost/asio/write.hpp>
#include <future>

Notification_server::Notification_server(const std::vector<boost::asio::ip::tcp::endpoint> &addresses)
	: gui_thread_private{.work = static_cast<decltype(gui_thread_private.work)>(shared.io_service), .server = {}} {
	notification_thread_private.listeners.reserve(addresses.size());
	std::transform(std::begin(addresses), std::end(addresses), std::back_inserter(notification_thread_private.listeners),
				   [this](boost::asio::ip::tcp::endpoint endpoint) {
					   return std::make_unique<Notification_thread_private::Listener>(shared.io_service, endpoint, notification_thread_private.sockets);
				   });
	gui_thread_private.server = std::thread{[&shared = shared] { shared.io_service.run(); }};
}

Notification_server::~Notification_server() {
	shared.io_service.stop();
	gui_thread_private.server.join();
}

void Notification_server::send_notification(std::string data) {
	shared.io_service.dispatch([data = std::make_shared<std::string>(std::move(data)), &sockets = notification_thread_private.sockets] {
		for (auto &socket : sockets) {
			boost::asio::async_write(*socket, boost::asio::buffer(data->c_str(), data->size()),
									 [data, &socket = socket, &sockets = sockets](const boost::system::error_code &error, std::size_t) {
										 if (error) {
											 socket->shutdown(boost::asio::ip::tcp::socket::shutdown_send);
											 socket->close();
											 sockets.erase(std::find(std::begin(sockets), std::end(sockets), socket));
										 }
									 });
		}
	});
}

std::size_t Notification_server::get_number_of_established_connections() {
	std::promise<std::size_t> sockets_size_promise;
	auto sockets_size_future = sockets_size_promise.get_future();
	shared.io_service.dispatch([&sockets = notification_thread_private.sockets, &sockets_size_promise] { sockets_size_promise.set_value(sockets.size()); });
	return sockets_size_future.get();
}

Notification_server::Notification_thread_private::Listener::Listener(boost::asio::io_service &io_service, boost::asio::ip::tcp::endpoint endpoint,
																	 std::vector<std::unique_ptr<boost::asio::ip::tcp::socket>> &sockets)
	: endpoint{endpoint}
	, io_service{io_service}
	, acceptor{io_service, endpoint}
	, socket{io_service} {
	struct Writer {};
	struct Acceptor {
		void operator()(const boost::system::error_code &error) {
			if (!error) {
				listener->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_receive);
				sockets->push_back(std::make_unique<boost::asio::ip::tcp::socket>(std::move(listener->socket)));
			}
			listener->acceptor.async_accept(listener->socket, listener->endpoint, Acceptor{sockets, listener});
		}
		std::vector<std::unique_ptr<boost::asio::ip::tcp::socket>> *sockets;
		Listener *listener;
	};
	Acceptor{&sockets, this}(boost::system::error_code{-1, boost::system::native_ecat});
}
