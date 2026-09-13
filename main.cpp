#include "limit_order_book.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

struct Message {
    uint64_t time_ns;
    uint64_t order_id;
    uint32_t shares;
    uint32_t price;
    int type;
    Side side;
};

std::string next_field(const std::string &line, size_t &pos) {
    if (pos < line.size() && line[pos] == ',') {
        ++pos;
    }

    const size_t comma = line.find(',', pos);
    const std::string field = line.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos);
    pos = comma == std::string::npos ? line.size() : comma;
    return field;
}

std::vector<Message> load_messages(const std::string &path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("failed to open message file: " + path);
    }

    std::vector<Message> messages;
    messages.reserve(1'000'000);

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        size_t pos = 0;
        const double time_seconds = std::stod(line, &pos);

        const int type = std::stoi(next_field(line, pos));
        const uint64_t order_id = std::stoull(next_field(line, pos));
        const uint32_t shares = static_cast<uint32_t>(std::stoul(next_field(line, pos)));
        const uint32_t price = static_cast<uint32_t>(std::stoul(next_field(line, pos)));
        const int direction = std::stoi(next_field(line, pos));

        messages.push_back(Message{
            .time_ns = static_cast<uint64_t>(time_seconds * 1e9),
            .order_id = order_id,
            .shares = shares,
            .price = price,
            .type = type,
            .side = direction > 0 ? Side::Buy : Side::Sell,
        });
    }

    return messages;
}

uint64_t percentile(std::vector<uint64_t> &sorted_ns, double p) {
    const size_t idx = static_cast<size_t>(p * static_cast<double>(sorted_ns.size() - 1));
    return sorted_ns[idx];
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " <path/to/message.csv>\n";
        return 1;
    }

    const std::vector<Message> messages = load_messages(argv[1]);
    std::cout << "loaded " << messages.size() << " messages\n";

    Book book;
    std::vector<uint64_t> op_latencies_ns;
    op_latencies_ns.reserve(messages.size());

    size_t skipped = 0;
    size_t rejected = 0;

    const auto t_start = std::chrono::steady_clock::now();
    for (const Message &msg : messages) {
        const auto op_start = std::chrono::steady_clock::now();

        try {
            switch (msg.type) {
            case 1:
                book.add_order(msg.order_id, msg.side, msg.shares, msg.price, msg.time_ns);
                break;
            case 2:
                book.cancel_order(msg.order_id, msg.shares, msg.time_ns);
                break;
            case 3:
                book.delete_order(msg.order_id);
                break;
            case 4:
                book.execute_order(msg.order_id, msg.shares, msg.time_ns);
                break;
            default:
                ++skipped;
                continue;
            }
        } catch (const std::exception &) {
            ++rejected;
            continue;
        }

        const auto op_end = std::chrono::steady_clock::now();
        op_latencies_ns.push_back(
            static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(op_end - op_start).count()));
    }
    const auto t_end = std::chrono::steady_clock::now();

    const auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t_end - t_start).count();
    const double elapsed_seconds = static_cast<double>(elapsed_ns) / 1e9;
    const double throughput = static_cast<double>(op_latencies_ns.size()) / elapsed_seconds;

    if (op_latencies_ns.empty()) {
        std::printf("processed:   0 (skipped %zu hidden/halt, rejected %zu unresolvable)\n", skipped, rejected);
        std::printf("wall time:   %.3f ms\n", elapsed_ns / 1e6);
        std::printf("no operations completed; skipping latency stats\n");
        return 0;
    }

    std::sort(op_latencies_ns.begin(), op_latencies_ns.end());

    std::printf("processed:   %zu (skipped %zu hidden/halt, rejected %zu unresolvable)\n", op_latencies_ns.size(),
                 skipped, rejected);
    std::printf("wall time:   %.3f ms\n", elapsed_ns / 1e6);
    std::printf("throughput:  %.0f msgs/sec\n", throughput);
    std::printf("latency p50: %llu ns\n", static_cast<unsigned long long>(percentile(op_latencies_ns, 0.50)));
    std::printf("latency p90: %llu ns\n", static_cast<unsigned long long>(percentile(op_latencies_ns, 0.90)));
    std::printf("latency p99: %llu ns\n", static_cast<unsigned long long>(percentile(op_latencies_ns, 0.99)));
    std::printf("latency p999: %llu ns\n", static_cast<unsigned long long>(percentile(op_latencies_ns, 0.999)));
    std::printf("latency max: %llu ns\n", static_cast<unsigned long long>(op_latencies_ns.back()));

    return 0;
}
