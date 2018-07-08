/* -*- c++ -*- */
/*
 * Copyright 2004 Free Software Foundation, Inc.
 * 
 * This file is part of GNU Radio
 * 
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

/*
 * config.h is generated by configure.  It contains the results
 * of probing for features, options etc.  It should be the first
 * file included in your .cc file.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "repartition_vectors_impl.h"

namespace gr {
  namespace dab {

repartition_vectors::sptr
repartition_vectors::make(size_t itemsize, unsigned int vlen_in, unsigned int vlen_out, unsigned int multiply, unsigned int divide)
{
  return gnuradio::get_initial_sptr
    (new repartition_vectors_impl(itemsize, vlen_in, vlen_out, multiply, divide));
}

repartition_vectors_impl::repartition_vectors_impl(size_t itemsize, unsigned int vlen_in, unsigned int vlen_out, unsigned int multiply, unsigned int divide)
  : gr::block("repartition_vectors",
             gr::io_signature::make (1, 1, itemsize*vlen_in),
             gr::io_signature::make (1, 1, itemsize*vlen_out)),
  d_itemsize(itemsize), d_vlen_in(vlen_in), d_vlen_out(vlen_out), d_multiply(multiply), d_divide(divide), d_synced(0)
{
  assert(vlen_in * multiply == vlen_out * divide);
  set_output_multiple(divide);
  set_tag_propagation_policy(TPP_DONT);
}

void 
repartition_vectors_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
{
  int in_req  = noutput_items / d_divide * d_multiply;

  unsigned ninputs = ninput_items_required.size ();
  for (unsigned i = 0; i < ninputs; i++)
    ninput_items_required[i] = in_req;
}

int 
repartition_vectors_impl::general_work (int noutput_items,
                        gr_vector_int &ninput_items,
                        gr_vector_const_void_star &input_items,
                        gr_vector_void_star &output_items)
{
  const char *iptr = (const char *) input_items[0];
  
  char *optr = (char *) output_items[0];

  int n_consumed = 0;
  int n_produced = 0;

  std::vector<int> tag_positions;
  int next_tag_position = -1;
  int next_tag_position_index = -1;

  // Get all stream tags with key "first", and make a vector of the positions.
  // "next_tag_position" contains the position within "iptr where the next "dab_sync" stream tag is found
  std::vector<tag_t> tags;
  get_tags_in_range(tags, 0, nitems_read(0), nitems_read(0) + ninput_items[0], pmt::mp("first"));
  for(int i=0;i<tags.size();i++) {
      int current;
      current = tags[i].offset - nitems_read(0);
      tag_positions.push_back(current);
      next_tag_position_index = 0;
  }
  if(next_tag_position_index >= 0) {
      next_tag_position = tag_positions[next_tag_position_index];
  }
  //


  while (d_synced==0 && ninput_items[0]>n_consumed) {
    if (next_tag_position == n_consumed) {
      // Action when stream tags is found:
      d_synced=1;
      //

      next_tag_position_index++;
      if (next_tag_position_index == tag_positions.size()) {
        next_tag_position_index = -1;
        next_tag_position = -1;
      }
      else {
        next_tag_position = tag_positions[next_tag_position_index];
      }
    }
    else {
      n_consumed++;
      iptr += d_vlen_in*d_itemsize;
    }
  }

  while (n_consumed + (int)d_multiply <= ninput_items[0] && n_produced + (int)d_divide <= noutput_items) {

    /* complete new block or is there already a next trigger? */
    for (unsigned int i=1; i<d_multiply; i++) {
      if (next_tag_position == n_consumed+i) { // If stream tag is fund here, then return so that a new call to work is made.
        n_consumed += i;
        consume_each(n_consumed);
        return n_produced;
      }
    }

    if (next_tag_position == n_consumed) {
      add_item_tag(0, nitems_written(0) + n_produced, pmt::intern("first"), pmt::intern(""), pmt::intern("repartition_vector"));

      next_tag_position_index++;
      if (next_tag_position_index == tag_positions.size()) {
        next_tag_position_index = -1;
        next_tag_position = -1;
      }
      else {
        next_tag_position = tag_positions[next_tag_position_index];
      }
    }

    /* data */
    memcpy(optr, iptr, d_multiply*d_itemsize*d_vlen_in);

    iptr += d_multiply * d_itemsize * d_vlen_in;
    optr += d_divide * d_itemsize * d_vlen_out;

    n_consumed += d_multiply;
    n_produced += d_divide;
  }

  consume_each(n_consumed);
  return n_produced;
}

}
}