#include "libff/algebra/curves/alt_bn128/alt_bn128_pp.hpp"
#include "libff/algebra/curves/bls12_377/bls12_377_pp.hpp"
#include "libff/algebra/curves/curve_serialization.hpp"
#include "libff/common/profiling.hpp"

#include <aio.h>
#include <exception>
#include <fcntl.h>
#include <fstream>
#include <map>
#include <sys/mman.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

using namespace libff;

static const size_t NUM_DIFFERENT_ELEMENTS = 1024;
static const size_t NUM_ELEMENTS_TO_READ = 1024 * 1024;
static const size_t NUM_ELEMENTS_IN_FILE = 256 * NUM_ELEMENTS_TO_READ;
static const size_t MAX_SPARSE_ELEMENT_INTERVAL =
    NUM_ELEMENTS_IN_FILE / NUM_ELEMENTS_TO_READ;

std::string get_filename(const std::string &identifier)
{
    return "group_elements_uncompressed_" + identifier + ".bin";
}

/// Returns true if the file was already present.
template<
    typename GroupT,
    form_t Form = form_montgomery,
    compression_t Comp = compression_off>
bool ensure_group_elements_file_uncompressed(const std::string &identifier)
{
    const std::string filename = get_filename(identifier);

    // If file doesn't exist, create it.
    struct stat s;
    if (stat(filename.c_str(), &s)) {
        std::cout << "  File '" << filename.c_str()
                  << "' does not exist. Creating ... ";
        std::flush(std::cout);

        // Fill a buffer with random elements
        std::vector<GroupT> elements;
        elements.reserve(NUM_DIFFERENT_ELEMENTS);
        for (size_t i = 0; i < NUM_DIFFERENT_ELEMENTS; ++i) {
            elements.push_back(GroupT::random_element());
        }

        // Use the buffer to fill the file
        std::ofstream out_s(
            filename.c_str(), std::ios_base::out | std::ios_base::binary);
        for (size_t i = 0; i < NUM_ELEMENTS_IN_FILE; ++i) {
            group_write<encoding_binary, Form, Comp>(
                elements[i % NUM_DIFFERENT_ELEMENTS], out_s);
        }
        out_s.close();

        std::cout << "Created\n";
        return false;
    }

    return true;
}

template<
    typename GroupT,
    form_t Form = form_montgomery,
    compression_t Comp = compression_off>
void profile_group_read_sequential_uncompressed(
    const std::string &identifier, const size_t)
{
    const std::string filename = get_filename(identifier);

    // Measure time taken to read the file
    std::cout << "  Sequential read '" << filename.c_str() << "' (expecting "
              << std::to_string(NUM_ELEMENTS_TO_READ) << " elements ...\n";
    {
        std::vector<GroupT> elements;
        elements.resize(NUM_DIFFERENT_ELEMENTS);

        std::ifstream in_s(
            filename.c_str(), std::ios_base::in | std::ios_base::binary);
        in_s.exceptions(
            std::ios_base::eofbit | std::ios_base::badbit |
            std::ios_base::failbit);

        {
            enter_block("Read group elements profiling");
            for (size_t i = 0; i < NUM_ELEMENTS_TO_READ; ++i) {
                group_read<encoding_binary, Form, Comp>(
                    elements[i % NUM_DIFFERENT_ELEMENTS], in_s);
            }
            leave_block("Read group elements profiling");
        }

        in_s.close();
    }
}

template<
    typename GroupT,
    form_t Form = form_montgomery,
    compression_t Comp = compression_off>
void profile_group_read_random_seek_uncompressed(const std::string &identifier)
{
    const std::string filename = get_filename(identifier);

    // Create a set of random indices. The i-th index to read from will be:
    //   (i + indices[i % NUM_DIFFERENT_ELEMENTS]) % NUM_ELEMENTS_IN_FILE
    std::vector<size_t> indices;
    indices.reserve(NUM_DIFFERENT_ELEMENTS);
    for (size_t i = 0; i < NUM_DIFFERENT_ELEMENTS; ++i) {
        indices.push_back(rand() % NUM_ELEMENTS_IN_FILE);
    }

    // Measure time taken to read the file
    std::cout << "  Random Access Read '" << filename.c_str() << "' ("
              << std::to_string(NUM_ELEMENTS_TO_READ) << " of "
              << std::to_string(NUM_ELEMENTS_IN_FILE) << " elements ...\n";
    {
        std::vector<GroupT> elements;
        elements.resize(NUM_DIFFERENT_ELEMENTS);

        std::ifstream in_s(
            filename.c_str(), std::ios_base::in | std::ios_base::binary);
        in_s.exceptions(
            std::ios_base::eofbit | std::ios_base::badbit |
            std::ios_base::failbit);

        const size_t element_size_on_disk = 2 * sizeof(elements[0].X);
        {
            enter_block("Read group elements profiling");
            for (size_t i = 0; i < NUM_ELEMENTS_TO_READ; ++i) {
                const size_t different_element_idx = i % NUM_DIFFERENT_ELEMENTS;
                const size_t idx =
                    (indices[different_element_idx] + i) % NUM_ELEMENTS_IN_FILE;
                const size_t offset = idx * element_size_on_disk;
                group_read<encoding_binary, Form, Comp>(
                    elements[different_element_idx], in_s.seekg(offset));
            }
            leave_block("Read group elements profiling");
        }

        in_s.close();
    }
}

template<
    typename GroupT,
    form_t Form = form_montgomery,
    compression_t Comp = compression_off>
void profile_group_read_random_seek_ordered_uncompressed(
    const std::string &identifier, const size_t interval)
{
    const std::string filename = get_filename(identifier);

    // Measure time taken to read the file
    std::cout << "  Random Access Seeks (Ordered) '" << filename.c_str()
              << "' (" << std::to_string(NUM_ELEMENTS_TO_READ) << " of "
              << std::to_string(NUM_ELEMENTS_IN_FILE) << " elements ...\n";
    {
        std::vector<GroupT> elements;
        elements.resize(NUM_DIFFERENT_ELEMENTS);

        std::ifstream in_s(
            filename.c_str(), std::ios_base::in | std::ios_base::binary);
        in_s.exceptions(
            std::ios_base::eofbit | std::ios_base::badbit |
            std::ios_base::failbit);

        const size_t element_size_on_disk = 2 * sizeof(elements[0].X);
        const size_t element_interval_bytes = element_size_on_disk * interval;
        {
            enter_block("Read group elements profiling");
            for (size_t i = 0; i < NUM_ELEMENTS_TO_READ; ++i) {
                const size_t different_element_idx = i % NUM_DIFFERENT_ELEMENTS;
                const size_t offset = i * element_interval_bytes;
                group_read<encoding_binary, Form, Comp>(
                    elements[different_element_idx], in_s.seekg(offset));
            }
            leave_block("Read group elements profiling");
        }

        in_s.close();
    }
}

template<typename GroupT>
void profile_group_read_random_seek_fd_uncompressed(
    const std::string &identifier, const size_t)
{
    const std::string filename = get_filename(identifier);

    // Create a set of random indices. The i-th index to read from will be:
    //   (i + indices[i % NUM_DIFFERENT_ELEMENTS]) % NUM_ELEMENTS_IN_FILE
    std::vector<size_t> indices;
    indices.reserve(NUM_DIFFERENT_ELEMENTS);
    for (size_t i = 0; i < NUM_DIFFERENT_ELEMENTS; ++i) {
        indices.push_back(rand() % NUM_ELEMENTS_IN_FILE);
    }

    // Measure time taken to read the file
    std::cout << "  Random Access Read '" << filename.c_str() << "' ("
              << std::to_string(NUM_ELEMENTS_TO_READ) << " of "
              << std::to_string(NUM_ELEMENTS_IN_FILE) << " elements ...\n";
    {
        std::vector<GroupT> elements;
        elements.resize(NUM_DIFFERENT_ELEMENTS);

        int f = open(filename.c_str(), O_RDONLY);
        if (f < 0) {
            throw std::runtime_error("failed to open " + filename);
        }

        const size_t element_size_on_disk = 2 * sizeof(elements[0].X);
        {
            enter_block("Read group elements profiling");
            for (size_t i = 0; i < NUM_ELEMENTS_TO_READ; ++i) {
                const size_t different_element_idx = i % NUM_DIFFERENT_ELEMENTS;
                const size_t idx =
                    (indices[different_element_idx] + i) % NUM_ELEMENTS_IN_FILE;
                const size_t offset = idx * element_size_on_disk;
                GroupT &dest_element = elements[different_element_idx];

                lseek(f, offset, SEEK_SET);
                read(f, &dest_element, element_size_on_disk);
            }
            leave_block("Read group elements profiling");
        }

        close(f);
    }
}

template<typename GroupT>
void profile_group_read_random_seek_fd_ordered_uncompressed(
    const std::string &identifier, const size_t)
{
    const std::string filename = get_filename(identifier);

    // Measure time taken to read the file
    std::cout << "  Random Access Read '" << filename.c_str() << "' ("
              << std::to_string(NUM_ELEMENTS_TO_READ) << " of "
              << std::to_string(NUM_ELEMENTS_IN_FILE) << " elements ...\n";
    {
        std::vector<GroupT> elements;
        elements.resize(NUM_DIFFERENT_ELEMENTS);

        int f = open(filename.c_str(), O_RDONLY);
        if (f < 0) {
            throw std::runtime_error("failed to open " + filename);
        }

        const size_t element_size_on_disk = 2 * sizeof(elements[0].X);
        const size_t element_interval_bytes =
            element_size_on_disk * NUM_ELEMENTS_IN_FILE / NUM_ELEMENTS_TO_READ;
        {
            enter_block("Read group elements profiling");
            for (size_t i = 0; i < NUM_ELEMENTS_TO_READ; ++i) {
                const size_t different_element_idx = i % NUM_DIFFERENT_ELEMENTS;
                const size_t offset = i * element_interval_bytes;
                GroupT &dest_element = elements[different_element_idx];

                lseek(f, offset, SEEK_SET);
                read(f, &dest_element, element_size_on_disk);
            }
            leave_block("Read group elements profiling");
        }

        close(f);
    }
}

template<typename GroupT>
void profile_group_read_random_seek_mmap_ordered_uncompressed(
    const std::string &identifier, const size_t interval)
{
    const std::string filename = get_filename(identifier);

    // Read sparse elements from a mem-mapped file, ensuring we always proceed
    // forwards.

    // Measure time taken to read the file
    std::cout << "  Random Access MMap (Ordered) '" << filename.c_str() << "' ("
              << NUM_ELEMENTS_TO_READ << " elements, interval: " << interval
              << ") ...\n";
    {
        std::vector<GroupT> elements;
        elements.resize(NUM_DIFFERENT_ELEMENTS);

        const size_t element_size_on_disk = 2 * sizeof(elements[0].X);
        const size_t file_size = NUM_ELEMENTS_IN_FILE * element_size_on_disk;

        int fd = open(filename.c_str(), O_RDONLY);
        if (fd < 0) {
            throw std::runtime_error("failed to open " + filename);
        }

        const void *file_base = mmap(
            nullptr,
            file_size,
            PROT_READ,
            MAP_PRIVATE /* | MAP_NOCACHE */,
            fd,
            0);
        if (MAP_FAILED == file_base) {
            throw std::runtime_error(
                std::string("mmap failed: ") + strerror(errno));
        }

        const size_t element_interval_bytes = element_size_on_disk * interval;

        {
            enter_block("Read group elements profiling");
            for (size_t i = 0; i < NUM_ELEMENTS_TO_READ; ++i) {
                const size_t different_element_idx = i % NUM_DIFFERENT_ELEMENTS;
                const size_t offset = i * element_interval_bytes;
                GroupT &dest_element = elements[different_element_idx];
                const GroupT *src =
                    (const GroupT *)(const void *)((size_t)file_base + offset);

                dest_element.X = src->X;
                dest_element.Y = src->Y;
            }
            leave_block("Read group elements profiling");
        }

        if (0 != munmap((void *)file_base, file_size)) {
            throw std::runtime_error("munmap failed");
        }
        close(fd);
    }
}

void cb_init(
    aiocb *cb, int fd, size_t offset_bytes, size_t size_bytes, void *dest)
{
    // std::cout << "cb_init: " << fd << ", off: " << offset_bytes << ", size: "
    // << size_bytes << ", dst: " << dest << "\n";

    memset(cb, 0, sizeof(aiocb));
    cb->aio_fildes = fd;
    cb->aio_offset = offset_bytes;
    cb->aio_buf = dest;
    cb->aio_nbytes = size_bytes;
    cb->aio_sigevent.sigev_notify = SIGEV_NONE;
    cb->aio_lio_opcode = LIO_READ;
}

void cb_enqueue(aiocb *cb)
{
    const int r = aio_read(cb);
    if (0 == r) {
        // std::cout << "cb_wait: enqueued\n";
        return;
    }

    throw std::runtime_error(
        std::string("error from aio_read: ") + strerror(errno));
}

ssize_t cb_wait(aiocb *cb)
{
    int err;
    for (;;) {
        err = aio_error(cb);
        if (err == EINPROGRESS) {
            std::this_thread::yield();
            // sleep(0);
            continue;
        }

        const ssize_t ret = aio_return(cb);
        if (ret >= 0) {
            // std::cout << "cb_wait: done\n";
            return ret;
        }

        if (err == ECANCELED) {
            throw std::runtime_error("aio_error: cancelled");
        }

        throw std::runtime_error(
            std::string("error from aio_error: ") + strerror(errno));
    }
}

template<size_t BatchSize> class batched_aio_reader
{
public:
    batched_aio_reader(int fd)
        : _fd(fd)
        , _active_batch(_batch1_ptrs)
        , _next_batch(_batch2_ptrs)
        , _next_batch_size(0)
    {
        memset(_batch1, 0, sizeof(_batch1));
        memset(_batch2, 0, sizeof(_batch2));
        for (size_t i = 0; i < BatchSize; ++i) {
            _batch1_ptrs[i] = &_batch1[i];
            _batch2_ptrs[i] = &_batch2[i];
        }
    }

    void enqueue_read_first_batch(
        size_t offset_bytes, size_t size_bytes, void *dest)
    {
        assert(_next_batch_size < BatchSize);

        aiocb *cb = _next_batch[_next_batch_size];
        cb_init(cb, _fd, offset_bytes, size_bytes, dest);

        // When first batch is full, enqueue it and allow writing to the next
        // batch.

        ++_next_batch_size;
        if (_next_batch_size == BatchSize) {
            enqueue_next_batch();

            std::swap(_active_batch, _next_batch);
            _next_batch_size = 0;
        }
    }

    void enqueue_read(size_t offset_bytes, size_t size_bytes, void *dest)
    {
        assert(_next_batch_size < BatchSize);

        aiocb *cb = _next_batch[_next_batch_size];
        cb_init(cb, _fd, offset_bytes, size_bytes, dest);

        ++_next_batch_size;
        if (_next_batch_size == BatchSize) {
            enqueue_next_batch();
        }
    }

    void wait_last_read()
    {
        // Wait for the _active_batch
        for (size_t i = 0; i < BatchSize; ++i) {
            int r = cb_wait(_active_batch[i]);
            if (0 > r) {
                throw std::runtime_error("bad read result");
            }
            if (0 == r) {
                std::cout << "+";
            }
        }

        // swap pointers so it can be written into
        std::swap(_active_batch, _next_batch);
        _next_batch_size = 0;
    }

protected:
    void enqueue_next_batch()
    {
        int r = lio_listio(LIO_NOWAIT, _next_batch, BatchSize, nullptr);
        if (r != 0) {
            throw std::runtime_error("enqueue_batch error");
        }
    }

    const int _fd;
    aiocb _batch1[BatchSize];
    aiocb _batch2[BatchSize];
    aiocb *_batch1_ptrs[BatchSize];
    aiocb *_batch2_ptrs[BatchSize];
    aiocb **_active_batch; // Next batch to wait for
    aiocb **_next_batch;   // Batch being filled
    size_t _next_batch_size;
};

/// Perform async reads of single group elements, with some (average) interval
/// between reads on disk, using the aio_* family of functions. Multple aio
/// requests are kept in-flight at the same time.
template<typename GroupT>
void profile_group_read_random_batch_aio_ordered_uncompressed(
    const std::string &identifier, const size_t interval)
{
    const std::string filename = get_filename(identifier);

    // Perform reads from a set of orderd, but random, locations where the
    // average interval between read locations (the sparsity) is `interval`.
    // Treat the file as being divided into sections, each of size `interval`.
    // For each section, we perform a single read from a random offset.

    // Preecompute the random offsets
    std::vector<size_t> section_offsets;
    section_offsets.reserve(NUM_ELEMENTS_TO_READ);
    for (size_t i = 0; i < NUM_ELEMENTS_TO_READ; ++i) {
        section_offsets.push_back((size_t)rand() * interval / RAND_MAX);
    }

    int fd = open(filename.c_str(), O_RDONLY);
    if (fd < 0) {
        throw std::runtime_error("failed to open " + filename);
    }

    const size_t BATCH_SIZE = 8;
    static_assert(
        0 == (NUM_ELEMENTS_TO_READ % BATCH_SIZE), "invalid batch size");
    batched_aio_reader<BATCH_SIZE> reader(fd);

    GroupT dest1[BATCH_SIZE];
    GroupT dest2[BATCH_SIZE];
    const size_t size_on_disk = 2 * sizeof(dest1[0].X);

    GroupT *cur_dest = dest1;
    GroupT *next_dest = dest2;

    std::cout << "  Deep Async Read '" << filename.c_str() << "' ("
              << std::to_string(NUM_ELEMENTS_TO_READ) << " of "
              << std::to_string(NUM_ELEMENTS_IN_FILE) << " elements ...\n";

    {
        enter_block("Read group elements profiling");

        size_t i = 0;

        // Enqueue first requests
        for (size_t j = 0; j < BATCH_SIZE; ++j) {
            reader.enqueue_read_first_batch(
                section_offsets[i + j] * size_on_disk,
                size_on_disk,
                cur_dest + j);
        }
        i += BATCH_SIZE;

        // Enqueue all requests
        for (; i < NUM_ELEMENTS_TO_READ; i += BATCH_SIZE) {
            // Enqueue next batch
            for (size_t j = 0; j < BATCH_SIZE; ++j) {
                reader.enqueue_read(
                    ((i + j) * interval + section_offsets[i + j]) *
                        size_on_disk,
                    size_on_disk,
                    cur_dest + j);
            }

            // Wait for current and process
            reader.wait_last_read();

            // Swap (at which point, cur_cb is the next element to wait for, and
            // next_cb is unused).
            std::swap(cur_dest, next_dest);
        }

        // Wait for last request
        reader.wait_last_read();

        leave_block("Read group elements profiling");
    }
}

typedef void (*profile_fn)(const std::string &, const size_t);

template<typename GroupT> class profile_selector
{
public:
    std::map<std::string, profile_fn> s_profile_functions = {
        {std::string("sequential"),
         profile_group_read_sequential_uncompressed<GroupT>},
        {std::string("stream"),
         profile_group_read_random_seek_ordered_uncompressed<GroupT>},
        {std::string("fd"),
         profile_group_read_random_seek_fd_ordered_uncompressed<GroupT>},
        {std::string("mmap"),
         profile_group_read_random_seek_mmap_ordered_uncompressed<GroupT>},
        {std::string("aio"),
         profile_group_read_random_batch_aio_ordered_uncompressed<GroupT>},
        {std::string("aio-batched"),
         profile_group_read_random_batch_aio_ordered_uncompressed<GroupT>},
    };
};

void usage(const char *const argv0)
{
    std::cout << "Usage: " << argv0 << " [flags]\n"
              << "\n"
              << "Flags:\n"
              << "  --interval <interval>       Use sparse interval (default "
              << MAX_SPARSE_ELEMENT_INTERVAL << ")\n"
              << "  --profile <profile name>    Run a specific profile "
                 "(default \"all\")\n";
}

template<typename GroupT>
void run_profile(
    const std::string &profile,
    const std::string &identifier,
    const size_t interval)
{
    std::cout << "profile: " << profile << "\n";
    std::cout << "identifier: " << identifier << "\n";

    if (!ensure_group_elements_file_uncompressed<GroupT>(identifier)) {
        return;
    }

    class profile_selector<GroupT> p;

    if (profile == "all") {
        for (const auto &pair : p.s_profile_functions) {
            run_profile<GroupT>(pair.first, identifier, interval);
        }
        return;
    }

    auto it = p.s_profile_functions.find(profile);
    if (it == p.s_profile_functions.end()) {
        throw std::runtime_error(std::string("no such profile: ") + profile);
    }

    it->second(identifier, interval);
}

int main(const int argc, char const *const *const argv)
{
    std::string profile = "all";
    size_t sparse_interval = MAX_SPARSE_ELEMENT_INTERVAL;

    for (size_t i = 1; i < (size_t)argc; ++i) {
        const char *const arg = argv[i];
        if (!strcmp(arg, "--interval")) {
            sparse_interval = std::stoi(std::string(argv[++i]));
        } else if (!strcmp(arg, "--profile")) {
            profile = argv[++i];
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    if (sparse_interval > MAX_SPARSE_ELEMENT_INTERVAL) {
        throw std::runtime_error("invalid interval");
    }

    // Some configurations are disabled for now.

    std::cout << "alt_bn128_pp\n";
    alt_bn128_pp::init_public_params();
    run_profile<alt_bn128_G1>(profile, "alt_bn128_G1", sparse_interval);
    run_profile<alt_bn128_G2>(profile, "alt_bn128_G2", sparse_interval);

    // For now (to avoid too many large data files and very long runtimes, this
    // is disabled. Uncomment to run for bls12-377.

    // std::cout << "bls12_377_pp\n";
    // bls12_377_pp::init_public_params();
    // run_profile<bls12_377_G1>("bls12_377_G1");
    // run_profile<bls12_377_G2>("bls12_377_G2");

    return 0;
}
