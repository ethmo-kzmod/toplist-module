#include "Include/session.hpp"

#include "toplist.h"
#include "Include/plog/Log.h"

void session::start() {

	for (int i = 0; i < max_length; ++i) {
		data_[i] = '\0';
	}

	socket_.async_read_some(boost::asio::buffer(data_, max_length),
		boost::bind(&session::handle_read, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));

}


void session::handle_read(const boost::system::error_code& error,
	size_t bytes_transferred) {

	if (!error)
	{ 

		std::string dataToSend(data_);

		PLOGD << "Data: " << dataToSend;

		Toplist::pToplist->sendDataToGame(dataToSend);

		socket_.async_read_some(boost::asio::buffer(data_, max_length),
			boost::bind(&session::handle_read, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}
	else
	{
		delete this;
	}
}