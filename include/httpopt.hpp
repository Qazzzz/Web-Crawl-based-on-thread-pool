#ifndef HTTPOPT_HPP
#define HTTPOPT_HPP

#include "common.h"

namespace http_get {

class HttpOpt {
public:
    HttpOpt();
    ~HttpOpt();
    HttpOpt(int port, int time_out_second);
    int get_opt(const string &get_url, string &response);
    int get_response_type();

protected:
    int create_socket();
    int connect_server(const string &ip);
    int request_opt(const string &ip, string &request, string &response);
    void set_timeout(int second);

private:
    class _HttpMsg;
    _HttpMsg* _http_msg_ptr;
    int _port;
    int _timeout_second;
};


class HttpOpt::_HttpMsg {
public:
    string head_info_;
    string body_info_;

    _HttpMsg() {
        _length_type = -1;
        _content_length = -2;
        _status = 0;
    };
    int length_type();
    int get_content_length();
    int get_status();
    int handle_chunked(int file_descriptor, string &recv_data);
    int handle_normal(int file_descriptor);
    int handle_unknown(int file_descriptor);

private:
    int _length_type; // 2: unknown  1: chunked  0: normal  -1: error or initial
    int _content_length; // >=0: normal  -1: error  -2: initial  -3: other types(chunked or unknown)
    int _status; // >0: normal  0: initial  -1: error 
};


/*-------------------------------
    Class HttpOpt
-------------------------------*/
HttpOpt::HttpOpt() {
    _port = 80;
    _http_msg_ptr = NULL;
    set_timeout(5);
}


HttpOpt::HttpOpt(int port, int timeout_second) {
    _port = port;
    _http_msg_ptr = NULL;
    set_timeout(timeout_second);
}

HttpOpt::~HttpOpt() {
    _http_msg_ptr = NULL;
}


void HttpOpt::set_timeout(int second) {
    _timeout_second = second;
}


int HttpOpt::create_socket() {
    int clientfile_descriptor;

    // Initial socket
    if ((clientfile_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    // Set the timeout option
    struct timeval timeout = {_timeout_second, 0};
    if (setsockopt(clientfile_descriptor, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        return -2;
    }
    if (setsockopt(clientfile_descriptor, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        return -3;
    }

    return clientfile_descriptor;
}

int HttpOpt::connect_server(const string &ip) {
    struct sockaddr_in server_dddr;
    int file_descriptor;
    if ((file_descriptor = create_socket()) < 0) {
        cout << "Create socket failed." << endl;
        return -1;
    }
    //cout << "Socket fd: " << file_descriptor << endl;

    // Construct the address
    bzero((char *)&server_dddr, sizeof(server_dddr));
    server_dddr.sin_family = AF_INET;
    server_dddr.sin_port = htons(_port);
    server_dddr.sin_addr.s_addr = inet_addr(ip.c_str());

    // Connect
    if (connect(file_descriptor, (struct sockaddr *) &server_dddr, sizeof(server_dddr)) < 0) {
        cout << "Fail to connect sever." << "(socket fd: " << file_descriptor << "）" << endl;
        return -2;
    }
    return file_descriptor;
}

int HttpOpt::get_response_type() {
    if (_http_msg_ptr != NULL) {
        return _http_msg_ptr->length_type();
    } else {
        return -2;
    }

}

int HttpOpt::request_opt(const string &ip, string &request, string &response) {
    // Connect to server
    int file_descriptor;
    if ((file_descriptor = connect_server(ip)) < 0) {
        return -1;
    }

    // Send operation data
    if (write(file_descriptor, request.c_str(), request.size()) < 0) {
        return -2;
    }

    // Receive data
    // Receive HTTP header
    char *response_buf = new char[MAX_BUF_SIZE];
    string recv_data;
    bool recv_head = false;
    size_t find_pos;
    _HttpMsg http_msg;
    _http_msg_ptr = &http_msg;
    int recv_data_count;
    while(1) {
        if ((recv_data_count = recv(file_descriptor, response_buf, 1, 0)) > 0) {
            recv_data.append(response_buf, recv_data_count);
            if (!recv_head) {
                if((find_pos = recv_data.find("\r\n\r\n")) != string::npos) {
                    http_msg.head_info_ = recv_data.substr(0, find_pos + 4);
                    recv_head = true;
                    break;
                }
            }
        } else {
        break;
        }
    }
    delete[] response_buf;
    response_buf = NULL;
    //cout << "HTTP header response = [" << http_msg.head_info_ << "]" <<endl;

    // Handle surplus data, !!! may throw error, need to be fixed !!!
    recv_data = recv_data.substr(find_pos + 4);

    // Receive HTTP body
    int return_value = 1;
    //cout << http_msg.get_status() << endl;
    if (http_msg.get_status() < 300 && http_msg.get_status() >= 200 ) {
        switch (http_msg.length_type()) {
        // Error
        case -1: {
            //cout << "HTTP response header's information error." << endl;
            return_value = -3; 
            break;
        }
        // Unkown
        case 2: {
            if (http_msg.handle_unknown(file_descriptor) == 1) {
                response = http_msg.body_info_;
                break;
            } else {
                return_value = -3;
                break;
            }
        }
        // Chunk
        case 1: {
            //cout << "The received HTTP header is coded with CHUNKED." << endl;
            if (http_msg.handle_chunked(file_descriptor, recv_data) == 1) {
                response = http_msg.body_info_;
                break;
            } else {
                return_value = -3;
                break;
            }
        }
        // Constant length
        case 0: {
            //cout << "The received HTTP header's length is " << http_msg.get_content_length() << endl;
            if (http_msg.handle_normal(file_descriptor) == 1) {
                //cout << "HTTP body response = [" << endl << http_msg.body_info_ << "]" <<endl;
                response = http_msg.body_info_;
                break;
            } else {
                return_value = -3;
                break;
            }
        }
        default: {
            return_value = -3;
            break;
        }
        }
    } else {
        std::stringstream ss;
        ss << http_msg.get_status();
        response = ss.str();
        return_value = -4;
    }
    //cout << return_value << endl;
    //cout << "HTTP body response = [" << endl << http_msg.body_info_ << "]" <<endl;

    if (close(file_descriptor) < 0) {
        if (return_value == 1) {
            return_value = -5;
        }
    } 
    return return_value;
}


int HttpOpt::get_opt(const string &get_url, string &response) {
    string hostname;
    string ip;
    string get_sub_url;

    // Fetch hostname & GET url
    int find_pos_1;
    int find_pos_2;
    if ((find_pos_1 = get_url.find("http://")) != string::npos) {
        if ((find_pos_2 = get_url.substr(find_pos_1 + 7).find_first_of("/")) != string::npos) {
            hostname = get_url.substr(find_pos_1 + 7, find_pos_2);
            get_sub_url = get_url.substr(find_pos_2 + 7);
        } else {
            hostname = get_url.substr(find_pos_1 + 7);
            get_sub_url.append("/");
        }
    } else {
        if ((find_pos_2 = get_url.find_first_of("/")) != string::npos) {
            hostname = get_url.substr(0, find_pos_2);
            get_sub_url = get_url.substr(find_pos_2);
        } else {
            hostname = get_url;
            get_sub_url.append("/");
        }
    }

    // DNS
    struct hostent *host_info = new struct hostent;
    struct hostent *result;
    int h_errnop;
    char *dns_buf = new char[DNS_BUF_SIZE];
    // !!Note: gethostbyname is not reentrant function
    if ((gethostbyname_r(hostname.c_str(), host_info, dns_buf, DNS_BUF_SIZE, &result, &h_errnop) != 0) ||
        h_errnop != 0) {
        cout << "DNS error " << "(code: " << h_errnop << ")"<< endl;
        return -1;
    }
 
    char *ip_char = new char[128];
    if (inet_ntop(AF_INET, *result->h_addr_list, ip_char, 128) == NULL) {
        return -1;
    } else {
        ip.assign(ip_char);
        //cout << ip << endl;
    }
    delete ip_char;
    ip_char = NULL;
    delete dns_buf;
    dns_buf = NULL;
    delete host_info;
    host_info = NULL;
    
    
    // Format GET request
    string request;
    request.append("GET ").append(get_sub_url).append(" HTTP/1.1").append("\r\n");
    request.append("Accept: */*").append("\r\n");
    request.append("Accept-Encoding: *").append("\r\n");
    request.append("Referer: ").append(get_url).append("\r\n");
    request.append("User-Agent: Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; Win64; x64; Trident/5.0)").append("\r\n");
    request.append("Host: ").append(hostname).append("\r\n");
    //request.append("Connection: Keep-Alive").append("\r\n");
    //request.append("Cache-Control: no-cache").append("\r\n");
    request.append("\r\n");
    //cout << "request = [" <<"IP: " << ip << endl << request << "]" << endl;

    // Make request call
    int status = request_opt(ip, request, response);
    if(status == 1) {
        return 1;
    } else if (status == -4) {
        cout << "HTTP status code: " << response << endl;
        return -2;
    } else if (status <= -1){
        cout << "Connection error. Error code: " << status << endl;
        return -3;
    }
}


/*-------------------------------
    Class httpOpt::httpMsg
-------------------------------*/
// 2: unknown  1: chunked  0: normal  -1: error or initial
int HttpOpt::_HttpMsg::length_type() {
    if (head_info_.empty()) {
        return -1;
    }
    if (_length_type == 2 || _length_type == 1 || _length_type == 0) {
        return _length_type;
    }

    // "Content-Length" can be ignored when it and "Transfer-Encoding" occur in the same time
    size_t find_pos1;
    size_t find_pos2;
    if ((find_pos1 = head_info_.find("Transfer-Encoding")) != string::npos) {
        if ((find_pos2 = head_info_.find("chunked"), find_pos1) != string::npos) {
            _length_type = 1;
            return 1;
        } else {
            _length_type = 2;
            return 2;
        }
    } else if ((find_pos1 = head_info_.find("Content-Length")) != string::npos) {
        _length_type = 0;
        return 0;
    } else {
        _length_type = 2;
        return 2;
    }
}


// >=0: normal  -1: error  -2: initial  -3: other types(chunked or unknown)
int HttpOpt::_HttpMsg::get_content_length() {
    if (_content_length >= -1) {
        return _content_length;
    }
    // _length_type == 0 indicate constant length
    if (_length_type != 0) {
        _content_length = -3;
        return -3;
    }
  
    size_t find_pos_1 = head_info_.find("Content-Length");
    if(find_pos_1 != string::npos) {
        size_t find_pos_2 = head_info_.find_first_of("0123456789", find_pos_1);
        size_t find_pos_3 = head_info_.find("\r\n", find_pos_1);
        if(find_pos_2 != string::npos && find_pos_3 != string::npos) {
            int length = atoi(head_info_.substr(find_pos_2, find_pos_3 - find_pos_2).c_str());
            _content_length = length;
            return length;
        }
    }
    return -1;
}


// >0: normal  0: initial  -1: error
int HttpOpt::_HttpMsg::get_status() {
    if (head_info_.empty()) {
        return -1;
    }
    if (_status > 0) {
        return _status;
    }

    size_t find_pos_1 = head_info_.find(" ");
    size_t find_pos_2 = head_info_.find_last_of(" ");
    if (find_pos_1 != string::npos && find_pos_2 != string::npos && find_pos_2-find_pos_1>0) {
        _status = atoi(head_info_.substr(find_pos_1, find_pos_2-find_pos_1).c_str());
        return _status;
    }
    return -1;
}


int HttpOpt::_HttpMsg::handle_chunked(int file_descriptor, string &recv_data) {
    if(_length_type != 1) {
        return -1;
    }

    size_t find_pos_1;
    size_t find_pos_2;
    size_t find_pos_3;
    int one_chunk_size = 0;
    int chunk_count = 1;
    int recv_data_count;
    char *response_buf = new char[MAX_BUF_SIZE];

    while (1) {
    // Receive Chunk head
        while(1) {
            if ((recv_data_count = recv(file_descriptor, response_buf, 1, 0)) > 0) {
                recv_data.append(response_buf, recv_data_count);
                if (((find_pos_1 = recv_data.find("\r\n")) != string::npos) &&
                    ((find_pos_2 = recv_data.find_first_of("0123456789abcdef")) != string::npos)) {
                    // Handle the chunk-extension if it exist
                    if ((find_pos_3 = recv_data.find_first_of(";", find_pos_2)) != string::npos) {
                        one_chunk_size = strtol(recv_data.substr(find_pos_3, find_pos_3 - find_pos_2).c_str(), (char**)NULL, 16);
                        //cout << "Chunk " << chunk_count << " Size: " << one_chunk_size << endl;
                        break;
                    } else {
                        one_chunk_size = strtol(recv_data.substr(find_pos_2, find_pos_1 - find_pos_2).c_str(), (char**)NULL, 16);
                        //cout << "Chunk " << chunk_count << " Size: " << one_chunk_size << endl;
                        break;
                    }
                }

            } else {
                cout << "HTTP chunk header parse error." << endl;
                delete[] response_buf;
                response_buf = NULL;
                return -1;
            }
        }
        chunk_count += 1;
        recv_data.clear();
        // Receive Chunk body
        if (one_chunk_size != 0) { 
            while (one_chunk_size > MAX_BUF_SIZE) {    
                if ((recv_data_count = recv(file_descriptor, response_buf, MAX_BUF_SIZE, 0)) > 0) {
                    body_info_.append(response_buf, recv_data_count);
                    one_chunk_size -= recv_data_count;
                } else {
                    cout << "HTTP chunk body receive error." << endl;
                    delete[] response_buf;
                    response_buf = NULL;
                    return -1;
                }
            }
            while (one_chunk_size > 0) {
                if ((recv_data_count = recv(file_descriptor, response_buf, one_chunk_size, 0)) > 0) {
                    body_info_.append(response_buf, recv_data_count);
                    one_chunk_size -= recv_data_count;
                } else {
                    cout << "HTTP chunk body receive error." << endl;
                    delete[] response_buf;
                    response_buf = NULL;
                    return -1;
                }
            }
            // Handle the '\r\n' after the Chunk body
            while (1) {
                if ((recv_data_count = recv(file_descriptor, response_buf, 2, 0)) > 0) {
                    recv_data.append(response_buf, recv_data_count);
                    if (recv_data.find("\r\n") != string::npos) {
                        break;
                    } else {
                        continue;
                    }
                } else {
                    cout << "HTTP chunk body stopsign receive error." << endl;
                    delete[] response_buf;
                    response_buf = NULL;
                    return -1;
                }
            }
            recv_data.clear();
        } else {
            break;
        }
    }
    
    //cout<<body_info_.size()<<endl;
    //cout<<body_info_<<endl;
    //cout<<recv_data_<<endl;
    delete[] response_buf;
    response_buf = NULL;
    return 1;
}


int HttpOpt::_HttpMsg::handle_normal(int file_descriptor) {
    if(_length_type != 0) {
        return -1;
    }
    if(_content_length == -2) {
        if(get_content_length() < 0) {
            return -1;
        }
    }
  
    int result = -1;
    int recv_data_count;
    int length = _content_length;
    char *response_buf = new char[MAX_BUF_SIZE];

    while(length > MAX_BUF_SIZE) {
        if((recv_data_count = recv(file_descriptor, response_buf, MAX_BUF_SIZE, 0)) > 0) {
            body_info_.append(response_buf, recv_data_count);
            length -= recv_data_count;
        } else {
            result = -1;
            break;
        }
    }
    while(length != 0) {
        if((recv_data_count = recv(file_descriptor, response_buf, 1, 0)) > 0) {
            body_info_.append(response_buf, recv_data_count);
            length -= recv_data_count;
        } else {
            result = -1;
            break;
        }
    }
    // Ensure the "Content-Length" == HTTP message length
    if(body_info_.size() != _content_length) {
        result = -1;
    }
  
    result = 1;
    delete[] response_buf;
    response_buf = NULL;
    return result;
}

int HttpOpt::_HttpMsg::handle_unknown(int file_descriptor) {
    if(_length_type != 2) {
        return -1;
    }

    int recv_data_count;
    char *response_buf = new char[MAX_BUF_SIZE];
    while(1) {
        if((recv_data_count = recv(file_descriptor, response_buf, MAX_BUF_SIZE, 0)) > 0) {
            body_info_.append(response_buf, recv_data_count);
        } else {
            delete[] response_buf;
            response_buf = NULL;
            return -1;
        }
    }

    delete[] response_buf;
    response_buf = NULL;
    return 1;
}

}; // namespace http_get

#endif // HttpOpt.hpp
