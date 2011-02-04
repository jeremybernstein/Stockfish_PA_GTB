/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2010 Marco Costalba, Joona Kiiski, Tord Romstad

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


////
//// Includes
////
#include <fstream>
#include <vector>

#include "search.h"
#include "thread.h"
#include "ucioption.h"

using namespace std;

////
//// Variables
////

static const string BenchmarkPositions[] = {
	"8/7p/6pB/1pK1k3/2b4P/8/8/8 b - - 2 43",
	"2rb1rk1/pp1n1p2/4b1pp/3Np2q/4P1n1/1B3NB1/PPP1Q1PP/4RR1K w - - 6 23",
	"2r2rk1/3n2Np/pp4p1/3P4/2P3N1/6R1/PB4PK/1b6 b - - 5 31",
	"5k2/5p2/p1QB2r1/2P5/2bP4/P5Pp/2q4P/5RK1 b - - 2 51",
	"1k1r3r/1p5p/p1p3p1/5p1P/Nb6/4BB2/1P3PP1/R5K1 b - - 1 24",
	"r2qnrk1/3nb1p1/5pp1/3PpP2/ppb5/4B3/PPPQN2P/1KNR1B1R w - - 0 20",
	"8/pp4k1/4pb2/7b/2P3p1/PKN2nP1/1P2Q1B1/8 b - - 0 41",
	"k3r3/q7/3Brp1p/3pn1p1/Q4RP1/1P6/P6P/2R3K1 w - - 4 36",
	"1k1r4/2p1n3/1pn1q3/p4pR1/2P2P1p/2B1PB2/P1Q2P1P/5K2 w - - 2 32",
	"3k4/1N2r1p1/pP3p1p/2K4P/P4p2/4B3/6P1/8 b - - 0 43",
	"3k4/p1p2p2/8/1P1N4/1P2KPP1/3B4/7r/4b3 w - - 0 34",
	"1r4k1/p5p1/4pp1p/3nN3/P1R2P2/1P4P1/5PKP/8 w - - 0 39",
	"8/1p3p1k/2p1b2p/p7/Pb5q/5R2/1P4P1/2Q4K w - - 6 36",
	"8/2b5/2Pk4/2nP4/1R6/1P6/2K5/8 w - - 1 57",
	"2b5/2kq4/1ppn1p1p/p1p5/2P1PP1P/PP2NKN1/8/3Q4 b - - 0 40",
	"8/8/3k1p2/2pb4/4p1PP/pP1pK3/P2N4/8 w - - 0 41",
	"1r4k1/1R3pp1/3q4/1Qp5/4n1B1/8/6NK/8 w - - 5 56",
	"3n1rk1/r2q4/p2p2Pn/2pPp3/2P4N/2NB2P1/P2Q1P2/1R4K1 b - - 4 38",
	"4r1k1/1b6/1N5p/2p2NP1/p5P1/P7/4rn1K/2R3R1 b - - 0 35",
	"5nrk/4r2p/3p4/2qPp2R/p1P5/3B1RP1/7P/1Q5K w - - 2 39",
	"6k1/5pp1/2P4p/3Q4/3R4/8/P1PK2P1/1q1r4 w - - 7 43",
	"4rk1r/qp1b1ppp/p2p4/4pN2/1P2P1n1/3Q4/P1PR1PPP/1K3B1R b - - 4 20",
	"3rr1k1/5ppp/p7/1p2q3/2nb4/P1P5/6PP/RNBQ1R1K w - - 0 22",
	"2r2r2/p3p1k1/1p1q2p1/4P3/P2p2P1/3Q4/1PPN2bb/R1R4K w - - 0 35",
	"4r3/5R2/p1kp4/7p/3P4/6P1/P1P4P/K7 b - - 0 31",
	"R7/3k2p1/4npP1/7P/1P2p3/3rN3/8/2K5 w - - 8 48",
	"3r4/1p1Pkpp1/8/p1r1pBnP/P5P1/1R6/1PP5/1K1R4 b - - 8 40",
	"2R5/5ppk/8/2pp1P2/1p2nQ2/1P5P/6P1/q6K w - - 11 45",
	"7k/5r1p/rnb1q1p1/3pPNR1/pQpP1P2/5B2/1PP4P/R6K w - - 1 32",
	"8/5p1k/4q2p/8/4b1pP/3Q2B1/5PPK/8 w - - 5 40",
	"8/3Q1pk1/3N1bp1/7p/1p2p3/1q2P1P1/5PKP/8 b - - 1 41",
	"R7/5pkp/1pp1p1p1/4q3/2r4P/6P1/P2Q1PB1/6K1 b - - 0 29",
	"2r5/6rk/7p/p1NP4/1P1Q4/6qP/4R1P1/6K1 b - - 2 48",
	"r1q1r1k1/1pp2pbp/3pbnp1/1Pn1p3/P3P3/3P2PP/1BQNNPBK/R4R2 b - - 4 22",
	"q1kr3r/1ppn1pp1/p2bp2P/P2P1b2/R1BN4/1PN1B2n/2PQ1PK1/4R3 b - - 2 22",
	"r1b2r2/6kp/2pq2p1/2N1p3/PpQ2p2/2PP2P1/7P/1R3RK1 w - - 0 26",
	"1r5k/4n2p/3qPn2/pQ1pp1r1/4Pp2/P2N1PpP/4B1P1/2RR2K1 w - - 0 31",
	"2r3k1/5p1p/4pb2/5q2/5P2/8/PP1Q2PP/3KRR2 b - - 2 31",
	"rn1R1r1k/4N2p/5ppb/q5N1/4Q2P/8/PP3PP1/1K5R w - - 1 24",
	"8/2p2ppk/3p4/p1r1q3/P1P1Pn1p/5P1P/B1R3PK/1Q6 w - - 10 46",
	"3b4/8/5k1p/4p3/1P3p2/3B1K2/5PPP/8 w - - 2 43",
	"8/2n2k2/p5pB/P2qp2p/2r1p2R/7P/6PK/1Q6 w - - 7 47",
	"2rb4/pp4pk/1q1n1rp1/3pN3/3P4/1P1P3Q/PB5P/R4RK1 b - - 4 25",
	"8/1p1r1pbk/pB4p1/P6p/3p1N1P/5PP1/q1rR1NK1/4Q3 w - - 8 36",
	"1r6/5k2/2b1p2p/8/1p4RP/8/P1B5/1K6 w - - 4 38",
	"4nr2/1kpnq3/bp1p2BP/3P4/2PBP1p1/2P5/4Q1P1/R5K1 w - - 0 33",
	"3qr1k1/pp3pP1/2p1r2p/3p1N2/2PP2P1/P1Q1B2P/6K1/5R2 b - - 3 31",
	"3qrk2/1p3pp1/r2b4/p2Pp2Q/P1P5/1P1n2PP/3B1P2/4RRK1 w - - 0 29",
	"r1br4/1p3pk1/p1n1p2p/2N2q1B/7Q/8/2PR2PP/4KR2 b - - 1 27",
	"2kr3r/ppbn1pp1/2p3p1/3pP3/3P1PPn/1P1B3P/1P1BN3/2KR1R2 b - - 0 22",
	"5r1k/6pp/8/1N3p2/2B2P2/2p3P1/R2b2KP/8 w - - 1 44",
	"8/1pk2pp1/p2np1p1/4Q1P1/3P1P2/7P/PP4BK/2q5 w - - 4 32",
	"1q6/6k1/4p1p1/2QpP2p/3P2r1/8/8/5R1K b - - 13 65",
	"6k1/r3r1p1/8/8/2R2P2/p7/8/K2R4 w - - 0 45",
	"5r2/1pqbppbk/r2p2np/p1pP3n/2P1P1P1/PP1BBN1P/3QN3/1R3RK1 b - - 0 23",
	"8/2q2kp1/1p2pp1p/r7/2P5/PQ1NR1nP/6P1/6K1 b - - 5 44",
	"3r4/1p2n2k/3qP1pp/5r2/1P1p4/1B6/1B1Q1PPP/R5K1 w - - 1 28",
	"4rnk1/1pqbppb1/6p1/p6P/7P/1NP2Q2/P1PB4/K2R3R b - - 0 28",
	"8/p7/1p6/1P3k2/P4bpB/1P1K1p2/7P/8 w - - 0 42",
	"2rr2k1/1p3ppp/p3b3/qn1p4/3P4/P1P1RB2/1B1Q1PPP/4R1K1 w - - 4 20",
    "8/k7/8/4Kn2/8/P7/8/1n6 b - - 0 1", 
    ""
};


////
//// Functions
////

/// benchmark() runs a simple benchmark by letting Stockfish analyze a set
/// of positions for a given limit each.  There are five parameters; the
/// transposition table size, the number of search threads that should
/// be used, the limit value spent for each position (optional, default
/// is ply 12), an optional file name where to look for positions in fen
/// format (default are the BenchmarkPositions defined above) and the type
/// of the limit value: depth (default), time in secs or number of nodes.
/// The analysis is written to a file named bench.txt.

void benchmark(int argc, char* argv[]) {

  vector<string> positions;
  string ttSize, threads, valStr, posFile, valType, mpv;
  int val, secsPerPos, maxDepth, maxNodes;

  ttSize  = argc > 2 ? argv[2] : "128";
  threads = argc > 3 ? argv[3] : "1";
  valStr  = argc > 4 ? argv[4] : "12";
  posFile = argc > 5 ? argv[5] : "default";
  valType = argc > 6 ? argv[6] : "depth";
  mpv     = argc > 7 ? argv[7] : "1";

  Options["Hash"].set_value(ttSize);
  Options["Threads"].set_value(threads);
  Options["OwnBook"].set_value("false");
  Options["Use Search Log"].set_value("true");
  Options["Search Log Filename"].set_value("bench.txt");
  Options["MultiPV"].set_value(mpv);

  secsPerPos = maxDepth = maxNodes = 0;
  val = atoi(valStr.c_str());

  if (valType == "depth" || valType == "perft")
      maxDepth = val;
  else if (valType == "time")
      secsPerPos = val * 1000;
  else
      maxNodes = val;

  if (posFile != "default")
  {
      ifstream fenFile(posFile.c_str());
      if (!fenFile.is_open())
      {
          cerr << "Unable to open positions file " << posFile << endl;
          exit(EXIT_FAILURE);
      }
      string pos;
      while (fenFile.good())
      {
          getline(fenFile, pos);
          if (!pos.empty())
              positions.push_back(pos);
      }
      fenFile.close();
  } else
      for (int i = 0; !BenchmarkPositions[i].empty(); i++)
          positions.push_back(BenchmarkPositions[i]);

  vector<string>::iterator it;
  int cnt = 1;
  int64_t totalNodes = 0;
  int startTime = get_system_time();

  for (it = positions.begin(); it != positions.end(); ++it, ++cnt)
  {
      Move moves[1] = { MOVE_NONE };
      int dummy[2] = { 0, 0 };
      Position pos(*it, false, 0);
      cerr << "\nBench position: " << cnt << '/' << positions.size() << endl << endl;
      if (valType == "perft")
      {
          int64_t perftCnt = perft(pos, maxDepth * ONE_PLY);
          cerr << "\nPerft " << maxDepth << " result (nodes searched): " << perftCnt << endl << endl;
          totalNodes += perftCnt;
      } else {
          if (!think(pos, false, false, dummy, dummy, 0, maxDepth, maxNodes, secsPerPos, moves))
              break;
          totalNodes += pos.nodes_searched();
      }
  }

  cnt = get_system_time() - startTime;
  cerr << "==============================="
       << "\nTotal time (ms) : " << cnt
       << "\nNodes searched  : " << totalNodes
       << "\nNodes/second    : " << (int)(totalNodes/(cnt/1000.0)) << endl << endl;

  // Under MS Visual C++ debug window always unconditionally closes
  // when program exits, this is bad because we want to read results before.
  #if (defined(WINDOWS) || defined(WIN32) || defined(WIN64))
  cerr << "Press any key to exit" << endl;
  cin >> ttSize;
  #endif
}
