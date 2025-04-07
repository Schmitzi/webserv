#include "../include/Client.hpp"
#include "../include/Server.hpp"
#include "../include/Webserv.hpp"

Client::Client() : _webserv(NULL), _server(NULL) {

}

Client::Client(Client const &other) {
    setWebserv(other._webserv);
    setServer(other._server);
}

Client::~Client() {

}

void Client::setWebserv(Webserv* webserv) {
    _webserv = webserv;
}

void Client::setServer(Server *server) {
    _server = server;
}

int Client::acceptConnection() {
    _addrLen = sizeof(_addr);
    _fd = accept(_server->_fd, (struct sockaddr *)&_addr, &_addrLen);
    if (_fd < 0) {
        _webserv->ft_error("Accept failed");
        return 1; // Try again
    }
    return 0;
}

void Client::displayConnection() {
    _ip = (unsigned char *)&_addr.sin_addr.s_addr;
    std::cout << BLUE << _webserv->getTimeStamp() << "New connection from " << _ip[0] << "." << _ip[1] \
        << "." << _ip[2] << "." << _ip[3] << ":" << ntohs(_addr.sin_port) << "\n";
}

int Client::recieveData() {
    memset(_buffer, 0, sizeof(_buffer));
    
    // Receive data
    int bytes_read = recv(_fd, _buffer, sizeof(_buffer) - 1, 0);
    
    if (bytes_read > 0) {
        std::cout << GREEN << "Recieved from " << _fd << ": " << _buffer << "\n" << RESET;

        // Echo the command back to client
        std::string response_str = "Server received: ";
        response_str += _buffer;
        send(_fd, response_str.c_str(), response_str.length(), 0);
        return 0;
    } 
    else if (bytes_read == 0) {
        // Connection closed by client
        std::cout << RED << "Client disconnected: " << _fd << RESET << "\n";
        return 1;
    }
    else {
        // Error occurred
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // This is not a real error, just no data available yet
            return 0;  // Return 0 to indicate "try again later"
        } else {
            // This is an actual error
            _webserv->ft_error("recv() failed");
            return 1;
        }
    }
}
