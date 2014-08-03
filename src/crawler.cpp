#include "../include/http_crawler.hpp"

int main(int argc, char **argv) {
    if (argc != 3) {
        cout << "Arrgument Error." << endl;
        return 0;
    }
    string url;
    map<string, int> url_list;

    url.assign(argv[1]);
    http_get::HttpCrawlerManager http_crawl_manager(atoi(argv[2]));
    http_crawl_manager.search_url(url);

    url_list = http_crawl_manager.get_url_list();
    map<string, int>::iterator map_it;
    for (map_it = url_list.begin(); map_it != url_list.end(); map_it++) {
        cout << map_it->first << "  ---(type: " << map_it->second << ")---" << endl;
    }

    return 0;
}