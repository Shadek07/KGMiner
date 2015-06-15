//
// Created by Baoxu Shi on 6/12/15.
//

#ifndef GBPEDIA_SOCKET_SERVER_H
#define GBPEDIA_SOCKET_SERVER_H

#include "graph.h"

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

#include <string>
#include <sstream>
#include <vector>

namespace local = boost::asio::local;

void worker(local::stream_protocol::socket *socket, graph<std::string, std::string>& g) {
  std::cout << "hello socket\n";
  boost::array<char, 1024> buf;
  boost::system::error_code error;
  size_t len = boost::asio::read(*socket, boost::asio::buffer(buf), error);

  std::vector<std::string> commands;
  std::stringbuf strbuf;
  for(int i = 0; i < len; i++) {
    if (buf.elems[i] != ',' && buf.elems[i] != ' ') {
      strbuf.sputc(buf.elems[i]);
    } else {
      commands.push_back(strbuf.str());
      strbuf.str("");
    }
  }
  commands.push_back(strbuf.str());

  std::string return_string = "";

  if (commands.size() == 0) {
    return_string = "No command provided\n";
  }else if (commands.at(0) == "path") {
    std::cout << (unsigned int)stoi(commands.at(1)) << " " << commands.at(1)<< " "
              << (unsigned int)stoi(commands.at(2)) << " " << commands.at(2)<< " "
              << (unsigned int)stoi(commands.at(3)) << " " << commands.at(3) << std::endl;
    std::vector< std::vector<unsigned int> > paths = g.dfs((unsigned int)stoi(commands.at(1)),
                                                           (unsigned int)stoi(commands.at(2)),
                                                           (unsigned int)stoi(commands.at(3)));
    std::cout << "find " << paths.size() << " paths\n";
    for(auto it = paths.cbegin(); it != paths.cend(); ++it) {
      for(auto itt = it->cbegin(); itt != it->cend(); ++itt) {
        std::cout << *itt << " ";
      }
      std::cout << "\n";
    }
    return_string = "Supported command\n";
  } else {
    return_string = "Unsupported command\n";
  }


  boost::asio::write(*socket, boost::asio::buffer(return_string));
  socket->close();
  delete socket;
}


class socket_server {

  boost::asio::io_service worker_io_service;
  boost::thread_group threadpool;
  boost::asio::io_service::work *work;

public:

  socket_server(int nworkers = 10) {
    work = new boost::asio::io_service::work(worker_io_service);
    for(int i = 0 ; i < nworkers; i++) {
      threadpool.create_thread(boost::bind(&boost::asio::io_service::run, &worker_io_service));
    }

    std::cout << threadpool.size() << " threads are created\n";
  }

  void start(std::string socket_name, graph<std::string, std::string>& g) {
    boost::asio::io_service socket_io_service;
    ::unlink(socket_name.c_str());
    local::stream_protocol::endpoint ep(socket_name);
    local::stream_protocol::acceptor acceptor(socket_io_service, ep);

    while(true) { //TODO: Throw Broken pipe error when all workers are busy and a lot of requests are coming.
      local::stream_protocol::socket *socket = new local::stream_protocol::socket(socket_io_service);
      acceptor.accept(*socket);
      std::cout << "get a connection!\n";
      worker_io_service.post(boost::bind(worker, socket, g));
      std::cout << "job posted!\n";
    }
  }

  void cleanup(){
    worker_io_service.stop();
    threadpool.join_all();
  }


};

#endif //GBPEDIA_SOCKET_SERVER_H