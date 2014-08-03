#include "../include/http_crawler.hpp"
#include "gtest/gtest.h"

namespace {

class HttpCrawlerTest: public ::testing::Test {
protected:
    virtual void SetUp() {
        web_site_1 = "family.baidu.com";              // Constant-length
        web_site_2 = "erp.baidu.com";                 // Constant-length
        web_site_3 = "hetu.baidu.com";                // Chunk
        web_site_4 = "family.baidu.com/qqqqq";        // 404
        web_site_5 = "qaz";                           // invalid input
        web_site_6 = "";                              // empty input
    }

    string web_site_1;
    string web_site_2;
    string web_site_3;
    string web_site_4;
    string web_site_5;
    string web_site_6;
};

} // namespace

TEST_F(HttpCrawlerTest, http_opt_costant_length) {
    http_get::HttpCrawlerManager http_crawl_manager_1;
    EXPECT_EQ(1, http_crawl_manager_1.search_url(web_site_1));

    http_get::HttpCrawlerManager http_crawl_manager_2;
    EXPECT_EQ(1, http_crawl_manager_2.search_url(web_site_2));
}

TEST_F(HttpCrawlerTest, http_opt_chunk) {
    http_get::HttpCrawlerManager http_crawl_manager_1;
    EXPECT_EQ(1, http_crawl_manager_1.search_url(web_site_3));
}

TEST_F(HttpCrawlerTest, http_opt_error_status_code) {
    http_get::HttpCrawlerManager http_crawl_manager_1;
    EXPECT_EQ(-1, http_crawl_manager_1.search_url(web_site_4));
}

TEST_F(HttpCrawlerTest, http_opt_error_input) {
    http_get::HttpCrawlerManager http_crawl_manager_1;
    EXPECT_EQ(-1, http_crawl_manager_1.search_url(web_site_5));

    http_get::HttpCrawlerManager http_crawl_manager_2;
    EXPECT_EQ(-1, http_crawl_manager_2.search_url(web_site_6));
}