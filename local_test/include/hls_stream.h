// Small stand-in for Xilinx hls_stream.h so kernels can be tested on a Mac.
// Like Vitis C simulation, dataflow functions run one after another, so the
// stream is just an unlimited queue.
// Extra checks:
//   - reading an empty stream stops the test. On the FPGA this would HANG.
//   - data left in a stream at the end is reported. That means the reader
//     and writer counts do not match.
#pragma once
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <string>

namespace hls {

inline bool g_stream_leftover = false;

template <typename T> class stream {
public:
    stream() : name_("unnamed_stream") {}
    explicit stream(const char *name) : name_(name) {}
    ~stream() {
        if (!q_.empty()) {
            fprintf(stderr,
                    "\n*** STREAM COUNT MISMATCH: '%s' still has %zu item(s) left at the end ***\n"
                    "    The writer wrote more than the reader read. Check your loop counts.\n",
                    name_.c_str(), q_.size());
            g_stream_leftover = true;
        }
    }

    void write(const T &v) { q_.push_back(v); }
    T read() {
        if (q_.empty()) {
            fprintf(stderr,
                    "\n*** STREAM READ WHILE EMPTY: '%s' ***\n"
                    "    The reader wants more data than the writer sent.\n"
                    "    On the FPGA this would HANG forever. Check your loop counts.\n",
                    name_.c_str());
            exit(3);
        }
        T v = q_.front();
        q_.pop_front();
        return v;
    }
    void read(T &v) { v = read(); }
    bool read_nb(T &v) {
        if (q_.empty()) return false;
        v = read();
        return true;
    }
    bool write_nb(const T &v) { write(v); return true; }
    bool empty() const { return q_.empty(); }
    bool full() const { return false; }
    size_t size() const { return q_.size(); }
    void operator<<(const T &v) { write(v); }
    void operator>>(T &v) { v = read(); }

private:
    std::string name_;
    std::deque<T> q_;
};

} // namespace hls
