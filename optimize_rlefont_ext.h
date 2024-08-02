// This implements the actual optimization passes of the compressor.

#include "datafile.hh"

namespace mcufont {
namespace rlefont {

typedef std::mt19937 rnd_t;
// Initialize the dictionary table with reasonable guesses.
void init_dictionary(DataFile &datafile);

// Perform a single optimization step, consisting itself of multiple passes
// of each of the optimization algorithms.
void optimize(DataFile &datafile, size_t iterations = 50);
void optimize_pass(DataFile &datafile, size_t &size, rnd_t &rnd, bool verbose);
void update_scores(mcufont::DataFile &datafile, bool verbose);

}}
