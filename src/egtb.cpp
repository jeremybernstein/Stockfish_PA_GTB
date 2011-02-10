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
#ifdef USE_EGTB
#include <cassert>
#include <string>

#include "egtb.h"
#include "egtb/gtb-probe.h"
#include "position.h"
#include "ucioption.h"


////
//// Local definitions
////

namespace
{
  /// Variables
  std::string TbPaths;
  int TbSize = 0;

  const char **paths;
  
  int CompressionScheme;
  
  bool Chess960;

  /// Local functions
  std::string trim(const std::string& str);
  int get_compression_scheme_from_string(const std::string& str);
}

////
//// Functions
////

// init_egtb() initializes or reinitializes gaviota tablebases if necessary.

void init_egtb()
{
  bool useTbs = Options["UseGaviotaTb"].value<bool>();
  std::string newTbPaths = Options["GaviotaTbPath"].value<std::string>();
  int newTbSize = Options["GaviotaTbCache"].value<int>();
  int newCompressionScheme = get_compression_scheme_from_string(Options["GaviotaTbCompression"].value<std::string>());
  Chess960 = Options["UCI_Chess960"].value<bool>();
    
  // If we don't use the tablebases, close them out (in case previously open).
  if (!useTbs) {
      close_egtb();
      return;
  }

  // Check if we need to initialize or reinitialize.
  if (newTbSize != TbSize || newTbPaths != TbPaths || newCompressionScheme != CompressionScheme)
  {
      // Close egtbs before reinitializing.
      close_egtb();

      TbSize = newTbSize;
      TbPaths = newTbPaths;
      CompressionScheme = newCompressionScheme;

      // Parse TbPaths string which can contain many paths separated by ';'
      paths = tbpaths_init();

      std::string substr;
      size_t prev_pos = 0, pos = 0;

      while( (pos = TbPaths.find(';', prev_pos)) != std::string::npos )
      {
          if ((substr = trim(TbPaths.substr(prev_pos, pos-prev_pos))) != "")
              paths = tbpaths_add(paths, substr.c_str());

          prev_pos = pos + 1;
      }

      if (prev_pos < TbPaths.size() && (substr = trim(TbPaths.substr(prev_pos))) != "")
          paths = tbpaths_add(paths, substr.c_str());

      //Finally initialize tablebases
      tb_init(0, CompressionScheme, paths);
      tbcache_init(TbSize * 1024 * 1024, 124);
      tbstats_reset();
  }
}

// close_egtb() closes/frees tablebases if necessary
void close_egtb()
{
  if  (TbSize != 0)
  {
      tbcache_done();
      tb_done();
      paths = tbpaths_done(paths);
      TbSize = 0;
  }
}

// probe_egtb() does the actual probing. On failure it returns VALUE_NONE.
Value probe_egtb(Position &pos, const int ply, const bool hard, const bool exact)
{
  // Conversion variables
  Bitboard occ;
  int count;

  // stockfish -> egtb
  int stm, epsquare, castling;
  unsigned int  ws[17], bs[17];
  unsigned char wp[17], bp[17];

  // egtb -> stockfish
  int tb_available;
  unsigned info = tb_UNKNOWN;
  unsigned pliestomate;

  // Prepare info for white (stockfish -> egtb)
  occ = pos.pieces_of_color(WHITE);
  count = 0;
  while (occ)
  {
      Square s = pop_1st_bit(&occ);
      ws[count] = s;
      wp[count] = (unsigned char) pos.type_of_piece_on(s);
      count++;
  }
  ws[count] = tb_NOSQUARE;
  wp[count] = tb_NOPIECE;

  // Prepare info for black (stockfish -> egtb)
  occ = pos.pieces_of_color(BLACK);
  count = 0;
  while (occ)
  {
      Square s = pop_1st_bit(&occ);
      bs[count] = s;
      bp[count] = (unsigned char) pos.type_of_piece_on(s);
      count++;
  }
  bs[count] = tb_NOSQUARE;
  bp[count] = tb_NOPIECE;

  // Prepare general info
  stm      = pos.side_to_move();
  epsquare = pos.ep_square();
  castling = tb_NOCASTLE;

  if (pos.can_castle(WHITE) || pos.can_castle(BLACK))
  {
      if (Chess960)
          return VALUE_NONE;

      if (pos.can_castle_kingside(WHITE))
          castling |= tb_WOO;
      if (pos.can_castle_queenside(WHITE))
          castling |= tb_WOOO;
      if (pos.can_castle_kingside(BLACK))
          castling |= tb_BOO;
      if (pos.can_castle_queenside(BLACK))
          castling |= tb_BOOO;
  }

  // Do the actual probing
  if (hard)
  {
      if (exact)
          tb_available = tb_probe_hard (stm, epsquare, castling, ws, bs, wp, bp, &info, &pliestomate);
      else
          tb_available = tb_probe_WDL_hard (stm, epsquare, castling, ws, bs, wp, bp, &info);
  }
  else
  {
      if (exact)
          tb_available = tb_probe_soft (stm, epsquare, castling, ws, bs, wp, bp, &info, &pliestomate);
      else
          tb_available = tb_probe_WDL_soft (stm, epsquare, castling, ws, bs, wp, bp, &info);
  }

  // Return probing info (if available)
  if (tb_available)
  {
    pos.set_tb_hits(pos.tb_hits() + 1);
    if (info == tb_DRAW)
        return VALUE_DRAW;
    else if (info == tb_WMATE && stm == tb_WHITE_TO_MOVE)
        return (exact ? value_mate_in(pliestomate+ply) : VALUE_KNOWN_WIN);
    else if (info == tb_BMATE && stm == tb_BLACK_TO_MOVE)
        return (exact ? value_mate_in(pliestomate+ply) : VALUE_KNOWN_WIN);
    else if (info == tb_WMATE && stm == tb_BLACK_TO_MOVE)
        return (exact ? value_mated_in(pliestomate+ply) : -VALUE_KNOWN_WIN);
    else if (info == tb_BMATE && stm == tb_WHITE_TO_MOVE)
        return (exact ? value_mated_in(pliestomate+ply) : -VALUE_KNOWN_WIN);
    else
        return VALUE_NONE;
  }
  else
      return VALUE_NONE;
}

namespace
{
  // trim() removes leading and trailing spaces (and like) from string

  std::string trim(const std::string& str) {
      size_t start = str.find_first_not_of(" \t\n\r");
      if(start == std::string::npos) return "";
      return str.substr(start, str.find_last_not_of(" \t\n\r") - start + 1);
  }
  
  int get_compression_scheme_from_string(const std::string& str) {
    if (str == "Uncompressed")
      return tb_UNCOMPRESSED;
    else if (str == "Huffman (cp1)")
      return tb_CP1;
    else if (str == "LZF (cp2)")
      return tb_CP2;
    else if (str == "Zlib-9 (cp3)")
      return tb_CP3;
    else
      return tb_CP4;
  }
}

#endif
