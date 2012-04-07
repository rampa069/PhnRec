#include <hash_map>
#inlcude "stkstream.h"


class labelstream
{

public:
	/// pointer to the stream
	istkstream * str;
}


class labelstore
{


public:

	typedef labelstore  __this_type;


	/**
	 *  @brief Registeres one MLF for use in application
	 *  @param fName MLF full filename
	 *  @return Pointer to this instance on success, otherwise NULL pointer
	 */
	__this_type *
	RegisterMLF(const string * fName);


	/**
	 *  @brief Gives access to the input stream
	 *  @return pointer to an istkstream which is set up by this class (propper
	 *          position, file, etc.)
	 */
	istkstream *
	input();


	/**
	 *  @brief Tries to open a specified label for reading
	 *  @label label to open
	 *  @return this on success, NULL on failure
	 */
	__this_type *
	open(const string & label);


	char *
	getline();



}; // class labelstore
