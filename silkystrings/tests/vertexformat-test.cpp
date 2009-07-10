/* This file is part of SilkyStrings 
 * Copyright (C) 2006  Olli Salli, Tuomas Perälä, Ville Virkkala
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <VertexFormat.h>

#include <cassert>
#include <algorithm>
#include <stdexcept>

bool
always(const SilkyStrings::VertexFormat::DataMemberSpec &ignored)
{
  return true;
}

int main()
{
  using namespace SilkyStrings;
  VertexFormat format;

  assert (format.get_size () == 0);
  assert (format.begin () == format.end ());
  
  assert (sizeof (float) == VertexFormat::size_of_type (VertexFormat::DATA_TYPE_FLOAT));

  format.insert (VertexFormat::DataMemberSpec (VertexFormat::DATA_ROLE_POS, VertexFormat::DATA_TYPE_FLOAT, 3, false));

  assert (format.get_size () == 3*sizeof (float));

  assert (std::count_if (format.begin (), format.end (), always) == 1);

  try
  {
    format.insert (VertexFormat::DataMemberSpec (VertexFormat::DATA_ROLE_POS, VertexFormat::DATA_TYPE_FLOAT, 4, false));
  }
  catch (std::logic_error &)
  {
    /* correct, should throw logic_error when trying to add multiple crack of same type */

    try
    {
      format.insert (VertexFormat::DataMemberSpec (VertexFormat::DATA_ROLE_FOG_COORD, VertexFormat::DATA_TYPE_USHORT, 42, false));
    }
    catch (std::logic_error &)
    {
      /* correct, should throw logic_error because the spec is invalid */
      return 0;
    }
  }
  catch (...)
  {
    /* incorrect, shouldn't throw anything else */
    assert (false);
  }

  /* should have went to the exception handlers, not here */
  assert (false);
}

