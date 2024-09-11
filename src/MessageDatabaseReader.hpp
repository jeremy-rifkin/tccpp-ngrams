#ifndef MESSAGEDATABASEREADER_HPP
#define MESSAGEDATABASEREADER_HPP

#include <string>
#include <thread>

#include <mongocxx/client.hpp>
#include <bsoncxx/json.hpp>
#include <rigtorp/SPSCQueue.h>

#include "utils.hpp"

struct MessageDatabaseEntry {
    sys_ms timestamp;
    std::string content;
};

class MessageDatabaseReader {
    mongocxx::cursor cursor;
    rigtorp::SPSCQueue<std::optional<MessageDatabaseEntry>> queue{1024};
    const string_set& private_channels;
    std::jthread read_thread;
    #ifdef TRACE
    int count = 0;
    #endif

public:
    MessageDatabaseReader(mongocxx::cursor cursor, const string_set& private_channels);

    std::optional<MessageDatabaseEntry> read();

private:
    MessageDatabaseEntry parse_document(const bsoncxx::document::view &doc);

    void reader();
};

#endif
