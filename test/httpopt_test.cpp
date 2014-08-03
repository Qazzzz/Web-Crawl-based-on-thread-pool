#include "../include/httpopt.hpp"
#include "gtest/gtest.h"

namespace {

class HttpOptTest: public ::testing::Test {
protected:
    virtual void SetUp() {
        web_site_1 = "family.baidu.com";              // Constant-length
        web_site_2 = "erp.baidu.com";                 // Constant-length
        web_site_3 = "hetu.baidu.com";                // Chunk
        web_site_4 = "family.baidu.com/qqqqq";        // 404
        web_site_5 = "qaz";                           // invalid input
        web_site_6 = "";                              // empty input
    }

    string response;
    string web_site_1;
    string web_site_2;
    string web_site_3;
    string web_site_4;
    string web_site_5;
    string web_site_6;
};

} // namespace

TEST_F(HttpOptTest, http_opt_costant_length) {
    http_get::HttpOpt http_opt(80, 5);
    EXPECT_EQ(1, http_opt.get_opt(web_site_1, response));

    http_get::HttpOpt http_opt_2(80, 5);
    EXPECT_EQ(1, http_opt.get_opt(web_site_2, response));
}

TEST_F(HttpOptTest, http_opt_chunk) {
    http_get::HttpOpt http_opt_1(80, 5);
    EXPECT_EQ(1, http_opt_1.get_opt(web_site_3, response));
}

TEST_F(HttpOptTest, http_opt_error_status_code) {
    http_get::HttpOpt http_opt_1(80, 5);
    EXPECT_EQ(-2, http_opt_1.get_opt(web_site_4, response));
}

TEST_F(HttpOptTest, http_opt_error_input) {
    http_get::HttpOpt http_opt_1(80, 5);
    EXPECT_EQ(-1, http_opt_1.get_opt(web_site_5, response));

    http_get::HttpOpt http_opt_2(80, 5);
    EXPECT_EQ(-1, http_opt_2.get_opt(web_site_6, response));
}


/*
int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
*/
