/** Definitions for the pqxx::result class and support classes.
 *
 * pqxx::result represents the set of result rows from a database query.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/result instead.
 *
 * Copyright (c) 2001-2017, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#ifndef PQXX_H_RESULT
#define PQXX_H_RESULT

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include <ios>
#include <stdexcept>

#include "pqxx/except"
#include "pqxx/field"
#include "pqxx/util"

/* Methods tested in eg. self-test program test001 are marked with "//[t01]"
 */

// TODO: Support SQL arrays

namespace pqxx
{
namespace internal
{
PQXX_LIBEXPORT void clear_result(const pq::PGresult *);

namespace gate
{
class result_connection;
class result_creation;
class result_row;
class result_sql_cursor;
} // namespace internal::gate
} // namespace internal


/// Result set containing data returned by a query or command.
/** This behaves as a container (as defined by the C++ standard library) and
 * provides random access const iterators to iterate over its rows.  A row
 * can also be accessed by indexing a result R by the row's zero-based
 * number:
 *
 * @code
 *	for (result::size_type i=0; i < R.size(); ++i) Process(R[i]);
 * @endcode
 *
 * Result sets in libpqxx are lightweight, reference-counted wrapper objects
 * which are relatively small and cheap to copy.  Think of a result object as
 * a "smart pointer" to an underlying result set.
 *
 * @warning The result set that a result object points to is not thread-safe.
 * If you copy a result object, it still refers to the same underlying result
 * set.  So never copy, destroy, query, or otherwise access a result while
 * another thread may be copying, destroying, querying, or otherwise accessing
 * the same result set--even if it is doing so through a different result
 * object!
 */
class PQXX_LIBEXPORT result
{
public:
  using size_type = result_size_type;
  using difference_type = result_difference_type;
  using reference = row;
  using const_iterator = const_result_iterator;
  using pointer = const_iterator;
  using iterator = const_iterator;
  using const_reverse_iterator = const_reverse_result_iterator;
  using reverse_iterator = const_reverse_iterator;

  result() noexcept : m_data(make_data_pointer()), m_query() {}		//[t03]
  result(const result &rhs) noexcept :					//[t01]
	m_data(rhs.m_data), m_query(rhs.m_query) {}

  result &operator=(const result &rhs) noexcept				//[t10]
  {
    m_data = rhs.m_data;
    m_query = rhs.m_query;
    return *this;
  }

  /**
   * @name Comparisons
   */
  //@{
  bool operator==(const result &) const noexcept;			//[t70]
  bool operator!=(const result &rhs) const noexcept			//[t70]
	{ return !operator==(rhs); }
  //@}

  const_reverse_iterator rbegin() const;				//[t75]
  const_reverse_iterator crbegin() const;
  const_reverse_iterator rend() const;					//[t75]
  const_reverse_iterator crend() const;

  const_iterator begin() const noexcept;				//[t01]
  const_iterator cbegin() const noexcept;
  inline const_iterator end() const noexcept;				//[t01]
  inline const_iterator cend() const noexcept;

  reference front() const noexcept;					//[t74]
  reference back() const noexcept;					//[t75]

  PQXX_PURE size_type size() const noexcept;				//[t02]
  PQXX_PURE bool empty() const noexcept;				//[t11]
  size_type capacity() const noexcept { return size(); }		//[t20]

  void swap(result &) noexcept;						//[t77]

  const row operator[](size_type i) const noexcept;			//[t02]
  const row at(size_type) const;					//[t10]

  void clear() noexcept { m_data.reset(); m_query.erase(); }		//[t20]

  /**
   * @name Column information
   */
  //@{
  /// Number of columns in result.
  PQXX_PURE row_size_type columns() const noexcept;			//[t11]

  /// Number of given column (throws exception if it doesn't exist).
  row_size_type column_number(const char ColName[]) const;		//[t11]

  /// Number of given column (throws exception if it doesn't exist).
  row_size_type column_number(const std::string &Name) const		//[t11]
	{return column_number(Name.c_str());}

  /// Name of column with this number (throws exception if it doesn't exist)
  const char *column_name(row_size_type Number) const;			//[t11]

  /// Type of given column
  oid column_type(row_size_type ColNum) const;				//[t07]
  /// Type of given column
  oid column_type(int ColNum) const					//[t07]
	{ return column_type(row_size_type(ColNum)); }

  /// Type of given column
  oid column_type(const std::string &ColName) const			//[t07]
	{ return column_type(column_number(ColName)); }

  /// Type of given column
  oid column_type(const char ColName[]) const				//[t07]
	{ return column_type(column_number(ColName)); }

  /// What table did this column come from?
  oid column_table(row_size_type ColNum) const;				//[t02]

  /// What table did this column come from?
  oid column_table(int ColNum) const					//[t02]
	{ return column_table(row_size_type(ColNum)); }

  /// What table did this column come from?
  oid column_table(const std::string &ColName) const			//[t02]
	{ return column_table(column_number(ColName)); }

  /// What column in its table did this column come from?
  row_size_type table_column(row_size_type ColNum) const;		//[t93]

  /// What column in its table did this column come from?
  row_size_type table_column(int ColNum) const				//[t93]
	{ return table_column(row_size_type(ColNum)); }

  /// What column in its table did this column come from?
  row_size_type table_column(const std::string &ColName) const		//[t93]
	{ return table_column(column_number(ColName)); }
  //@}

  /// Query that produced this result, if available (empty string otherwise)
  PQXX_PURE const std::string &query() const noexcept;			//[t70]

  /// If command was @c INSERT of 1 row, return oid of inserted row
  /** @return Identifier of inserted row if exactly one row was inserted, or
   * oid_none otherwise.
   */
  PQXX_PURE oid inserted_oid() const;					//[t13]

  /// If command was @c INSERT, @c UPDATE, or @c DELETE: number of affected rows
  /** @return Number of affected rows if last command was @c INSERT, @c UPDATE,
   * or @c DELETE; zero for all other commands.
   */
  PQXX_PURE size_type affected_rows() const;				//[t07]


private:
  using data_pointer = std::shared_ptr<const internal::pq::PGresult>;

  /// Underlying libpq result set.
   data_pointer m_data;

  /// Factory for data_pointer.
  static data_pointer make_data_pointer(
	const internal::pq::PGresult *res=nullptr)
	{ return data_pointer(res, internal::clear_result); }

  /// Query string.
  std::string m_query;

  friend class pqxx::field;
  PQXX_PURE const char *GetValue(size_type Row, row_size_type Col) const;
  PQXX_PURE bool GetIsNull(size_type Row, row_size_type Col) const;
  PQXX_PURE field_size_type GetLength(
	size_type,
	row_size_type) const noexcept;

  friend class pqxx::internal::gate::result_creation;
  result(internal::pq::PGresult *rhs, const std::string &Query);
  PQXX_PRIVATE void CheckStatus() const;

  friend class pqxx::internal::gate::result_connection;
  friend class pqxx::internal::gate::result_row;
  bool operator!() const noexcept { return !m_data.get(); }
  operator bool() const noexcept { return m_data.get(); }

  [[noreturn]] PQXX_PRIVATE void ThrowSQLError(
	const std::string &Err,
	const std::string &Query) const;
  PQXX_PRIVATE PQXX_PURE int errorposition() const noexcept;
  PQXX_PRIVATE std::string StatusError() const;

  friend class pqxx::internal::gate::result_sql_cursor;
  PQXX_PURE const char *CmdStatus() const noexcept;
};


/// Write a result field to any type of stream
/** This can be convenient when writing a field to an output stream.  More
 * importantly, it lets you write a field to e.g. a @c stringstream which you
 * can then use to read, format and convert the field in ways that to() does not
 * support.
 *
 * Example: parse a field into a variable of the nonstandard
 * "<tt>long long</tt>" type.
 *
 * @code
 * extern result R;
 * long long L;
 * stringstream S;
 *
 * // Write field's string into S
 * S << R[0][0];
 *
 * // Parse contents of S into L
 * S >> L;
 * @endcode
 */
template<typename CHAR>
inline std::basic_ostream<CHAR> &operator<<(
	std::basic_ostream<CHAR> &S, const pqxx::field &F)		//[t46]
{
  S.write(F.c_str(), std::streamsize(F.size()));
  return S;
}


/// Convert a field's string contents to another type
template<typename T>
inline void from_string(const field &F, T &Obj)				//[t46]
	{ from_string(F.c_str(), Obj, F.size()); }

/// Convert a field to a string
template<>
inline std::string to_string(const field &Obj)				//[t74]
	{ return std::string(Obj.c_str(), Obj.size()); }

} // namespace pqxx

#include "pqxx/result_iterator.hxx"

#include "pqxx/compiler-internal-post.hxx"

#endif
