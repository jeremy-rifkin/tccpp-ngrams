#ifndef MESSAGEDATABASEMANAGER_HPP
#define MESSAGEDATABASEMANAGER_HPP

#include <string>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <bsoncxx/json.hpp>

#include "MessageDatabaseReader.hpp"

class MessageDatabaseManager {
    string_set private_channels;
    mongocxx::instance inst;
    mongocxx::client connection;
    mongocxx::database db;

public:
    MessageDatabaseManager(const std::string& auth_url);

    MessageDatabaseReader make_message_database_reader();

private:
    bsoncxx::builder::basic::array private_channel_list() const;

    void load_channel_thread_stati();

    void register_channel_info(const bsoncxx::v_noabi::document::view &doc, std::string_view id_field);
};

#endif
