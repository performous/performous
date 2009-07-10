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

#ifndef __SS_VERTEXFORMAT_H_
#define __SS_VERTEXFORMAT_H_

#include "GL.h"

#include <typeinfo>
#include <stdexcept>
#include <set>

namespace SilkyStrings
{
  /**
   * The VertexFormat class represents the storage format of geometry vertices.
   */

  class VertexFormat
  {
    public:

      /**
       * Role of a data member.
       */

      enum DataRole
      {
        /** vertex position */
        DATA_ROLE_POS,
        
        /** vertex normal */
        DATA_ROLE_NORMAL,
        
        /** vertex color */
        DATA_ROLE_COLOR,

        /** secondary vertex color */
        DATA_ROLE_SECONDARY_COLOR,

        /** vertex texture coordinate 0 */
        DATA_ROLE_TEXCOORD0,
        /** vertex texture coordinate 1 */
        DATA_ROLE_TEXCOORD1,
        /** vertex texture coordinate 2 */
        DATA_ROLE_TEXCOORD2,
        /** vertex texture coordinate 3 */
        DATA_ROLE_TEXCOORD3,
        /** vertex texture coordinate 4 */
        DATA_ROLE_TEXCOORD4,
        /** vertex texture coordinate 5 */
        DATA_ROLE_TEXCOORD5,
        /** vertex texture coordinate 6 */
        DATA_ROLE_TEXCOORD6,
        /** vertex texture coordinate 7 */
        DATA_ROLE_TEXCOORD7,

        /** fog coordinate */
        DATA_ROLE_FOG_COORD,

        /** generic vertex attribute 0 */
        DATA_ROLE_ATTRIB0,
        /** generic vertex attribute 1 */
        DATA_ROLE_ATTRIB1,
        /** generic vertex attribute 2 */
        DATA_ROLE_ATTRIB2,
        /** generic vertex attribute 3 */
        DATA_ROLE_ATTRIB3,
        /** generic vertex attribute 4 */
        DATA_ROLE_ATTRIB4,
        /** generic vertex attribute 5 */
        DATA_ROLE_ATTRIB5,
        /** generic vertex attribute 6 */
        DATA_ROLE_ATTRIB6,
        /** generic vertex attribute 7 */
        DATA_ROLE_ATTRIB7,
        /** generic vertex attribute 8 */
        DATA_ROLE_ATTRIB8,
        /** generic vertex attribute 9 */
        DATA_ROLE_ATTRIB9,
        /** generic vertex attribute 10 */
        DATA_ROLE_ATTRIB10,
        /** generic vertex attribute 11 */
        DATA_ROLE_ATTRIB11,
        /** generic vertex attribute 12 */
        DATA_ROLE_ATTRIB12,
        /** generic vertex attribute 13 */
        DATA_ROLE_ATTRIB13,
        /** generic vertex attribute 14 */
        DATA_ROLE_ATTRIB14,
        /** generic vertex attribute 15 */
        DATA_ROLE_ATTRIB15,
        /** generic vertex attribute 16 */
        DATA_ROLE_ATTRIB16,
        /** generic vertex attribute 17 */
        DATA_ROLE_ATTRIB17,
        /** generic vertex attribute 18 */
        DATA_ROLE_ATTRIB18,
        /** generic vertex attribute 19 */
        DATA_ROLE_ATTRIB19,
        /** generic vertex attribute 20 */
        DATA_ROLE_ATTRIB20,
        /** generic vertex attribute 21 */
        DATA_ROLE_ATTRIB21,
        /** generic vertex attribute 22 */
        DATA_ROLE_ATTRIB22,
        /** generic vertex attribute 23 */
        DATA_ROLE_ATTRIB23,
        /** generic vertex attribute 24 */
        DATA_ROLE_ATTRIB24,
        /** generic vertex attribute 25 */
        DATA_ROLE_ATTRIB25,
        /** generic vertex attribute 26 */
        DATA_ROLE_ATTRIB26,
        /** generic vertex attribute 27 */
        DATA_ROLE_ATTRIB27,
        /** generic vertex attribute 28 */
        DATA_ROLE_ATTRIB28,
        /** generic vertex attribute 29 */
        DATA_ROLE_ATTRIB29,
        /** generic vertex attribute 30 */
        DATA_ROLE_ATTRIB30,
        /** generic vertex attribute 31 */
        DATA_ROLE_ATTRIB31
      };

      /**
       * Symbolic values for different data storage types.
       */

      enum DataType
      {
        DATA_TYPE_BYTE,
        DATA_TYPE_UBYTE,

        DATA_TYPE_SHORT,
        DATA_TYPE_USHORT,

        DATA_TYPE_INT,
        DATA_TYPE_UINT,

        DATA_TYPE_FLOAT,
        DATA_TYPE_DOUBLE
      };

      /**
       * At least 8 bits wide signed integer.
       */

      typedef GLbyte      BYTE;

      /**
       * At least 8 bits wide unsigned integer.
       */

      typedef GLubyte     UBYTE;

      /**
       * At least 16 bits wide signed integer.
       */

      typedef GLshort     SHORT;

      /**
       * At least 16 bits wide unsigned integer.
       */

      typedef GLushort    USHORT;

      /**
       * At least 32 bits wide signed integer.
       */

      typedef GLint       INT;

      /**
       * At least 32 bits wide unsigned integer.
       */

      typedef GLuint      UINT;

      /**
       * At least 32 bits wide floating point value.
       */

      typedef GLfloat     FLOAT;

      /**
       * At least 64 bits wide floating point value.
       */

      typedef GLdouble    DOUBLE;

      /** Get storage size of a data type.
       *
       * @param t The data type to query for.
       * @return The storage size of the given data type, in basic machine
       * units.
       */

      static size_t
      size_of_type (DataType t);

      /** Get corresponding OpenGL data type.
       *
       * @param t The data type to query for.
       * @return The corresponding OpenGL data type.
       */

      static GLenum
      to_gl_type (DataType t);

      /**
       * Struct DataMemberSpec contains information about a member of the
       * format.
       */

      struct DataMemberSpec
      {
        /** Constructor.
         *
         * @param role The role of the member.
         * @param type The storage data type of the member.
         * @param count How many scalar values should form the member.
         * @param dynamic If the member should be optimized for dynamic
         * updating.
         */

        inline DataMemberSpec (DataRole role,
                               DataType type,
                               int count,
                               bool dynamic = false)
          : role(role), type(type), count(count), dynamic(dynamic)
        {
          if (!is_valid ())
            throw std::logic_error ("This role/type/count combination is not supported");
        }

        /**
         * Strict weak ordering.
         */

        inline bool
        operator< (const DataMemberSpec &a) const
        {
          return role < a.role;
        }

        /**
         * Check if the spec specifies a valid member.
         *
         * @return The result of the check.
         */

        bool
        is_valid () const;

        /**
         * Usage role.
         */

        DataRole role;

        /**
         * Storage data type.
         */

        DataType type;

        /**
         * Scalar value count.
         */

        int count;

        /**
         * Optimized for dynamic updating.
         */

        bool dynamic;
      };

      /** Add a member specification to the format.
       *
       * Add the given data member specification to the format.
       * Only a single member for each DataRole can be added to a format.
       * Only POS is required, specs for other roles are optional.
       * Will throw on unsupported data types for formats and attempting to
       * add a data member for the same role twice.
       *
       * The supported storage data types and scalar value counts for different
       * roles are:
       *
       * POS:
       *    SHORT, INT, \b FLOAT or DOUBLE
       *    2, 3 or 4
       *
       * NORMAL:
       *    BYTE, SHORT, INT, \b FLOAT or DOUBLE
       *    3 only
       *
       * COLOR:
       *    BYTE, \b UBYTE, SHORT, USHORT, INT, UINT, \b FLOAT or DOUBLE
       *    3 or 4
       *
       * SECONDARY_COLOR:
       *    BYTE, \b UBYTE, SHORT, USHORT, INT, UINT, \b FLOAT or DOUBLE
       *    3 only
       *
       * TEXCOORDn:
       *    SHORT, INT, \b FLOAT or DOUBLE
       *    1, 2, 3 or 4
       *
       * FOG_COORD:
       *    FLOAT or DOUBLE
       *    1 only
       *
       * ATTRIBn:
       *    BYTE, \b UBYTE, SHORT, USHORT, INT, UINT, \b FLOAT or DOUBLE
       *    1, 2, 3 or 4
       *
       * The emphasis, if any, is on the types most likely leading to optimal
       * performance.
       *
       * @param spec The specification to add.
       */

      void
      insert (const DataMemberSpec &spec);

      /** Calculate the storage size of the format.
       *
       * @return The storage size of the format, in basic machine units.
       */

      size_t
      get_size () const;

      /**
       * Standard STL bidirectional output iterator for iterating through the
       * format.
       */

      typedef std::set<DataMemberSpec>::const_iterator const_iterator;

      /**
       * Returns a iterator pointing to the first member specification.
       */

      const_iterator
      begin () const;

      /**
       * Returns a one-past-the-end iterator.
       */

      const_iterator
      end () const;

    private:

      std::set<DataMemberSpec> members;
  };

  /**
   * The VertexDataElement class is a "Safe union" for vertex data.
   */

  class VertexDataElement
  {
    public:
      
      /**
       * BYTE constructor.
       */

      inline VertexDataElement (VertexFormat::BYTE val): type(VertexFormat::DATA_TYPE_BYTE), int8_val(val) {};

      /**
       * UBYTE constructor.
       */

      inline VertexDataElement (VertexFormat::UBYTE val): type(VertexFormat::DATA_TYPE_UBYTE), uint8_val(val) {};

      /**
       * SHORT constructor.
       */

      inline VertexDataElement (VertexFormat::SHORT val): type(VertexFormat::DATA_TYPE_SHORT), int16_val(val) {};

      /**
       * USHORT constructor.
       */

      inline VertexDataElement (VertexFormat::USHORT val): type(VertexFormat::DATA_TYPE_USHORT), uint16_val(val) {};

      /**
       * INT constructor.
       */

      inline VertexDataElement (VertexFormat::INT val): type(VertexFormat::DATA_TYPE_INT), int32_val(val) {};

      /**
       * UINT constructor.
       */

      inline VertexDataElement (VertexFormat::UINT val): type(VertexFormat::DATA_TYPE_UINT), uint32_val(val) {};

      /**
       * FLOAT constructor.
       */

      inline VertexDataElement (VertexFormat::FLOAT val): type(VertexFormat::DATA_TYPE_FLOAT), float_val(val) {};

      /**
       * DOUBLE constructor.
       */

      inline VertexDataElement (VertexFormat::DOUBLE val): type(VertexFormat::DATA_TYPE_DOUBLE), double_val(val) {};

      /**
       * Explicit type constructor.
       *
       * @param type The type to initialize the element to.
       * @param val The value to initialize with. It will be casted to the requested
       * format. It will be truncated in most cases.
       */

      inline VertexDataElement (VertexFormat::DataType type, VertexFormat::DOUBLE val)
        : type(type)
      {
        switch (type)
        {
          case VertexFormat::DATA_TYPE_BYTE:
            int8_val = (VertexFormat::BYTE (val));
            return;

          case VertexFormat::DATA_TYPE_UBYTE:
            uint8_val = (VertexFormat::BYTE (val));
            return;

          case VertexFormat::DATA_TYPE_SHORT:
            int16_val = (VertexFormat::SHORT (val));
            return;

          case VertexFormat::DATA_TYPE_USHORT:
            uint16_val = (VertexFormat::SHORT (val));
            return;

          case VertexFormat::DATA_TYPE_INT:
            int32_val = (VertexFormat::INT (val));
            return;

          case VertexFormat::DATA_TYPE_UINT:
            uint32_val = (VertexFormat::UINT (val));
            return;

          case VertexFormat::DATA_TYPE_FLOAT:
            float_val = (VertexFormat::FLOAT (val));
            return;

          case VertexFormat::DATA_TYPE_DOUBLE:
            double_val = (VertexFormat::DOUBLE (val));
            return;
        }
      }

      /**
       * Copying operator.
       */

      inline VertexDataElement (const VertexDataElement &a): type(a.type), double_val(a.double_val) {};

      /**
       * BYTE conversion operator.
       */

      inline operator VertexFormat::BYTE () const
      {
        if (type != VertexFormat::DATA_TYPE_BYTE)
          throw std::bad_cast ();

        return int8_val;
      }

      /**
       * UBYTE conversion operator.
       */

      inline operator VertexFormat::UBYTE () const
      {
        if (type != VertexFormat::DATA_TYPE_UBYTE)
          throw std::bad_cast ();

        return uint8_val;
      }

      /**
       * SHORT conversion operator.
       */

      inline operator VertexFormat::SHORT () const
      {
        if (type != VertexFormat::DATA_TYPE_SHORT)
          throw std::bad_cast ();

        return int16_val;
      }

      /**
       * USHORT conversion operator.
       */

      inline operator VertexFormat::USHORT () const
      {
        if (type != VertexFormat::DATA_TYPE_USHORT)
          throw std::bad_cast ();

        return uint16_val;
      }

      /**
       * INT conversion operator.
       */

      inline operator VertexFormat::INT () const
      {
        if (type != VertexFormat::DATA_TYPE_INT)
          throw std::bad_cast ();

        return int32_val;
      }

      /**
       * UINT conversion operator.
       */

      inline operator VertexFormat::UINT () const
      {
        if (type != VertexFormat::DATA_TYPE_UINT)
          throw std::bad_cast ();

        return uint32_val;
      }

      /**
       * FLOAT conversion operator.
       */

      inline operator VertexFormat::FLOAT () const
      {
        if (type != VertexFormat::DATA_TYPE_FLOAT)
          throw std::bad_cast ();

        return float_val;
      }

      /**
       * DOUBLE conversion operator.
       */

      inline operator VertexFormat::DOUBLE () const
      {
        if (type != VertexFormat::DATA_TYPE_DOUBLE)
          throw std::bad_cast ();

        return double_val;
      }

      /**
       * Get the data type of the element.
       *
       * @return The data type.
       */

      inline VertexFormat::DataType
      get_type () const
      {
        return type;
      }

    private:

      VertexFormat::DataType type;

      union
      {
        VertexFormat::BYTE int8_val;
        VertexFormat::UBYTE uint8_val;
        VertexFormat::SHORT int16_val;
        VertexFormat::USHORT uint16_val;
        VertexFormat::INT int32_val;
        VertexFormat::UINT uint32_val;
        VertexFormat::FLOAT float_val;
        VertexFormat::DOUBLE double_val;
      };
  };
};

#endif

