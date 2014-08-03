#ifndef HTTP_CRAWLER_HPP
#define HTTP_CRAWLER_HPP

#include "httpopt.hpp"
#include "thread_pool.hpp"

namespace http_get {

class CrawlJob;

class HttpCrawlerManager {
public:
    HttpCrawlerManager();
    explicit HttpCrawlerManager(int crawl_depth);
    ~HttpCrawlerManager();
    int search_url(string& url);
    map<string, int>& get_url_list();
    multi_thread::ThreadPool* get_thread_pool() const;

private:
    int _search_depth;
    multi_thread::ThreadPool* _thread_pool;
    map<string, int> _url_list;
};


//--------------------------------------
class HttpCrawler {
public:
    HttpCrawler();
    ~HttpCrawler();
    map<string, int> _url_list;
    void set_parent_crawler(HttpCrawler* parent_crawler);
    void set_supervisor(HttpCrawlerManager* supervisor);
    int crawl_url(const string &url, int search_depth);

    struct _JobArgs {
        string _url;
        int _search_depth;
    };
    
protected:
    void body_parser(string &response);
    void collect_result(map<string, int> &parent_url_list);

private:
    string _url;
    HttpCrawlerManager* _supervisor;
    vector<CrawlJob*> _crawl_job_list;
    vector<struct _JobArgs*> _job_args_list;
    HttpCrawler* _parent_crawler;
};


//----------------------------------------
class CrawlJob:public ::multi_thread::Job {
public:
    CrawlJob();
    ~CrawlJob();
    void set_http_crawler_supervisor(HttpCrawlerManager* supervisor);
    void run(void* args);
    HttpCrawler* get_crawler() const ;

private:
    HttpCrawler* _http_crawler;
};




/*-------------------------------
    Class HttpCrawlerManager
-------------------------------*/
HttpCrawlerManager::HttpCrawlerManager() {
    _search_depth = 3;
    _thread_pool = new multi_thread::ThreadPool;
}

HttpCrawlerManager::HttpCrawlerManager(int crawl_depth) {
    if (crawl_depth >= 0) {
        _search_depth = crawl_depth;
    } else {
        _search_depth = 3;
    }
    _thread_pool = new multi_thread::ThreadPool;
}

HttpCrawlerManager::~HttpCrawlerManager() {
    delete _thread_pool;
    _thread_pool = NULL;
}

int HttpCrawlerManager::search_url(string& url) {
    if (url.empty()) {
        cout << "Empty string." << endl;
        return -1;
    }
    HttpCrawler *http_crawler = new HttpCrawler;
    http_crawler->set_supervisor(this);
    int status = http_crawler->crawl_url(url, _search_depth);

    while (_thread_pool->get_running_thread() != 0) {
        sleep(1);
    }
    delete http_crawler;
    http_crawler = NULL;

    return status;
}

multi_thread::ThreadPool* HttpCrawlerManager::get_thread_pool() const {
    return _thread_pool;
}

map<string, int>& HttpCrawlerManager::get_url_list() {
    return _url_list;
}


/*-------------------------------
        Class HttpCrawler
-------------------------------*/
HttpCrawler::HttpCrawler() {
    _parent_crawler = NULL;
    _supervisor = NULL;
}

HttpCrawler::~HttpCrawler() {
    for (int i = _crawl_job_list.size() - 1; i >= 0; i--) {
        delete _crawl_job_list.at(i);
        _crawl_job_list.at(i) = NULL;
    }

    if (_parent_crawler != NULL) {
        collect_result(_parent_crawler->_url_list);
    } else if (_supervisor != NULL){
        collect_result(_supervisor->get_url_list());
    } else {
        cout << "Error." << endl;
    }
    
    for (int i = _job_args_list.size() - 1; i >= 0; i--) {
        delete _job_args_list.at(i);
        _job_args_list.at(i) = NULL;
    }
}

// 0:content-length  1:chunk  2:unknown  -3:initial  -1:unreachable or error
int HttpCrawler::crawl_url(const string &url, int search_depth) {
    if (url.find("http://") != string::npos) {
        _url = url.substr(7);
    } else {
        _url = url;
    }
    string response;
    map<string, int>::iterator map_it;
    HttpOpt http_opt(80, 5);
    _url_list[_url] = -3;

    if (http_opt.get_opt(url, response) == 1) {
        int response_type = http_opt.get_response_type();
        if (response_type != -2) {
            _url_list[_url] = response_type;
        } else {
            _url_list[_url] = -1;
        }
        if (search_depth > 0 ) {
            body_parser(response);
            for (map_it = _url_list.begin(); map_it != _url_list.end(); map_it++) {
                if (map_it->first == _url) {
                    continue;
                }
                CrawlJob* crawl_job_ptr = new CrawlJob;
                crawl_job_ptr->set_http_crawler_supervisor(_supervisor);
                _crawl_job_list.push_back(crawl_job_ptr);
                crawl_job_ptr->get_crawler()->set_parent_crawler(this);
                struct _JobArgs *job_args = new struct _JobArgs;
                _job_args_list.push_back(job_args);

                job_args->_url = map_it->first;
                job_args->_search_depth = search_depth - 1;

                _supervisor->get_thread_pool()->run(crawl_job_ptr, job_args);
            }
            return 1;
        } else {
            return 2;   // bottom
        }
    } else {
        for (map_it = _url_list.begin(); map_it != _url_list.end(); map_it++) {
            _url_list[_url] = -1;
        }
        return -1;
    }
}

// Parse URL in the body
void HttpCrawler::body_parser(string &response) {
    size_t find_pos_1;
    size_t find_pos_2;
    size_t find_pos_3;
    size_t find_pos_4;
    size_t find_pos_begin = 0;
    string temp_url;
    bool slash_flag;
    if (_url.at(_url.size() - 1) == '/') {
        slash_flag = true;
    } else {
        slash_flag = false;
    }

    // Fetch the whole urls in the response
    while (1) {
        if ((find_pos_1 = response.find("href=", find_pos_begin)) != string::npos) {
            if ((find_pos_2 = response.find_first_of("\"", find_pos_1 + 6)) != string::npos) {
                temp_url = _url;
                if ((response.find("http://", find_pos_1 + 6)) == find_pos_1 + 6) {
                    temp_url = response.substr(find_pos_1 + 13, find_pos_2 - find_pos_1 - 13);
                } else if (slash_flag && response.at(find_pos_1 + 6) == '/') {
                    temp_url.append(response.substr(find_pos_1 + 7, find_pos_2 - find_pos_1 - 7));
                } else if (!slash_flag && response.at(find_pos_1 + 6) == '/') {
                    temp_url.append(response.substr(find_pos_1 + 6, find_pos_2 - find_pos_1 - 6));
                } else if (slash_flag && response.at(find_pos_1 + 6) != '/') {
                    temp_url.append(response.substr(find_pos_1 + 6, find_pos_2 - find_pos_1 - 6));
                } else {
                    temp_url.append("/");
                    temp_url.append(response.substr(find_pos_1 + 6, find_pos_2 - find_pos_1 - 6));
                }
                // Uniform the urls
                if (temp_url.at(temp_url.size() - 1) == '/') {
                    temp_url.erase(temp_url.size() - 1, 1);
                }
                // Eliminate the duplication
                if (_url_list.find(temp_url) == _url_list.end()) {
                    // Eliminate the non-page URL
                    if (!((find_pos_3 = temp_url.find_last_of(".")) != string::npos &&
                         (find_pos_4 = temp_url.find_last_of("/")) != string::npos &&
                         (find_pos_3 > find_pos_4))
                       ) {
                        _url_list[temp_url] = -3; 
                    }
                }
                // Reset the start point of the search area
                find_pos_begin = find_pos_2;
            } else {
                find_pos_begin = find_pos_1 + 5;
                continue;
            }
        } else {
            break;
        }
    }
}

void HttpCrawler::set_supervisor(HttpCrawlerManager* supervisor) {
    _supervisor = supervisor;
}

void HttpCrawler::collect_result(map<string, int> &parent_url_list) {
    map<string, int>::iterator map_it;
    for (map_it = _url_list.begin(); map_it != _url_list.end(); map_it++) {
        parent_url_list[map_it->first] = map_it->second;
    }
}

void HttpCrawler::set_parent_crawler(HttpCrawler* parent_crawler) {
    if (parent_crawler != NULL) {
        _parent_crawler = parent_crawler;
    }
}


/*-------------------------------
        Class CrawlJob
-------------------------------*/
CrawlJob::CrawlJob() {
    _http_crawler = new HttpCrawler;
}

CrawlJob::~CrawlJob() {
    delete _http_crawler;
    _http_crawler = NULL;
}

void CrawlJob::run(void *args) {
    _http_crawler->crawl_url(static_cast<HttpCrawler::_JobArgs*>(args)->_url,
                                          static_cast<HttpCrawler::_JobArgs*>(args)->_search_depth);
}

void CrawlJob::set_http_crawler_supervisor(HttpCrawlerManager* supervisor) {
    _http_crawler->set_supervisor(supervisor);
}

HttpCrawler* CrawlJob::get_crawler() const {
    return _http_crawler;
}


} // namespace http_get

#endif // http_crawler.hpp