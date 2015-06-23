///
//	uty@uaty
///
#include "utils.h"

VOID wtoA(WCHAR* source,CHAR* dest)
{
	ULONG i;
	for (i = 0;i<wcslen(source)+1;i++){
		dest[i] = (CHAR)source[i];
	}
}
//--------------------------------------------------------------------
VOID Atow(CHAR* source,WCHAR* dest)
{
	ULONG	i;
	memset(dest,0,wcslen(dest));
	for(i = 0;i < strlen(source);i++){
		dest[i] = source[i];
	}
}
//--------------------------------------------------------------------
VOID GetArg(CHAR* commAndline,ULONG* Argc,CHAR* Argv[],ULONG	mAxArgc)
{
	ULONG	length;
	ULONG	i = 0;
	ULONG	index = 0;
	length = strlen(commAndline);
	Argv[index] = commAndline;
	index++;
	for(i = 0;i < length;i++){
		if(commAndline[i] == ' ' && commAndline[i+1] != ' ' && commAndline[i+1] != '\0'){
			commAndline[i] = '\0';//breAk up
			i++;
			while(commAndline[i] == ' ' && i<length){
				i++;
			}
			if(index < mAxArgc){				//////////////处理最大参数个数 只取前mAxArgc个参数
				Argv[index] = commAndline + i;
				index ++;
			}									////////////////////////
			if(commAndline[i] == '"'){   /////////////////////这段是对"的处理
				commAndline[i] = '\0';
				Argv[index-1] = (CHAR*)Argv[index-1]+1;
				while(commAndline[i] != '"' && i<length){
					i++;
				}
				if (commAndline[i] == '"'){
					commAndline[i] = '\0';
					i--;
				}
			}//if                                //////////////
		}
				
	}
	*Argc = index;
}
//--------------------------------------------------------------------
//from privAte\net\sockets\winsock2\dll\sinsock2\Addrconv.cpp
#define HTONS(s) ( ( ((s) >> 8) & 0x00FF ) | ( ((s) << 8) & 0xFF00 ) ) 
#define HTONL(l)                            \
	( ( ((l) >> 24) & 0x000000FFL ) |       \
	( ((l) >>  8) & 0x0000FF00L ) |       \
	( ((l) <<  8) & 0x00FF0000L ) |       \
	( ((l) << 24) & 0xFF000000L ) )
//--------------------------------------------------------------------
// these defines are used to check if address parts are in range
#define MAX_EIGHT_BIT_VALUE       0xff
#define MAX_SIXTEEN_BIT_VALUE     0xffff
#define MAX_TWENTY_FOUR_BIT_VALUE 0xffffff

// Defines for different based numbers in an address
#define BASE_TEN     10
#define BASE_EIGHT   8
#define BASE_SIXTEEN 16

#define INADDR_NONE             0xffffffff
unsigned long
inet_Addr (
           IN const char FAR * cp
           )
/*++
Routine Description:

    Convert a string containing an Internet Protocol dotted address into an
    in_addr.

Arguments:

    cp - A null terminated character string representing a number expressed in
         the Internet standard ".'' notation.

Returns:

    If no error occurs, inet_addr() returns an unsigned long containing a
    suitable binary representation of the Internet address given.  If the
    passed-in string does not contain a legitimate Internet address, for
    example if a portion of an "a.b.c.d" address exceeds 255, inet_addr()
    returns the value INADDR_NONE.

--*/
{
    ULONG value;                // value to return to the user
    ULONG number_base;          // The number base in use for an
                                 // address field
    ULONG address_field_count;  // How many fields where found in the
                                 // address string
    char c;                      // temp variable to hold the charater
                                 // that is being processed currently
    ULONG fields[4];            // an array of unsigned longs to
                                 // recieve the values from each field
                                 // in the address
    ULONG *p_fields = fields;   // a pointer used to index through
                                 // the 'fields' array
    BOOLEAN MoreFields = TRUE;      // Are there more address fields to scan

    if( cp == NULL ) {
        //SetLastError( WSAEFAULT );
        return INADDR_NONE;
    }

    while (MoreFields) {
//
//    Collect number up to ``.''.
//    Values are specified as for C:
//    0x=hex, 0=octal, other=decimal.
//
        value = 0;
        number_base = BASE_TEN;
        // Is the first charater '0' ?
        // The default number base is base ten. If the first charater in
        // an address field is '0' then the user is using octal or hex
        // notation for the assress field
        if (*cp == '0') {
            // If the second charater in the field is x or X then this is
            // a hex number else it is an octal number.
            if (*++cp == 'x' ||
                *cp == 'X') {
                number_base = BASE_SIXTEEN;
                cp++; // skip the x
            }
            else {
                number_base = BASE_EIGHT;
            }
        }

        // Process the charaters in the address string until a non digit
        // charater is found.
        c = *cp;
        while (c) {
            if (isdigit(c)) {
                value = (value * number_base) + (c - '0');
                cp++;
                c = *cp;
                continue;
            }
            if ((number_base == BASE_SIXTEEN) && isxdigit(c)) {
                value = (value << 4) + (c + 10 - (islower(c) ? 'a' : 'A'));
                cp++;
                c = *cp;
                continue;
            }
            break;
        }

        // Is the charater following the the number a '.'. If so skip the
        // the '.' and scan the next field.
        if (*cp == '.') {
            /*
             * Internet format:
             *  a.b.c.d
             *  a.b.c   (with c treated as 16-bits)
             *  a.b (with b treated as 24 bits)
             */
            if (p_fields >= fields + 3) {
                // and internet address cannot have more than 4 fields so
                // return an error
                return (INADDR_NONE);
            }
            // set the value of this part of the addess and advance
            // the pointer to the next part
            *p_fields++ = value;
            //
            cp++;
        }
        else {
            MoreFields=FALSE;
        } //else
    } //while

    //
    //  Check for trailing characters. A valid address can end with
    //  NULL or whitespace. An address may not end with a '.'
    //
    if ((*cp == '\0' && *(cp - 1) == '.') ||
        (*cp && !isspace(*cp))) {
        return (INADDR_NONE);
    }

    // set the the value of the final field in the address
    *p_fields++ = value;


    //
    // Concoct the address according to the number of fields
    // specified.
    //
    address_field_count = p_fields - fields;
    switch (address_field_count) {

      case 1:               // a -- 32 bits
        value = fields[0];
        break;

      case 2:               // a.b -- 8.24 bits
        if (fields[0] > MAX_EIGHT_BIT_VALUE ||
            fields[1] > MAX_TWENTY_FOUR_BIT_VALUE) {
            return (INADDR_NONE);
        } //if
        else {
            value = (fields[0] << 24) |
            (fields[1] & MAX_TWENTY_FOUR_BIT_VALUE);
        } //else
        break;

      case 3:               // a.b.c -- 8.8.16 bits
        if (fields[0] > MAX_EIGHT_BIT_VALUE ||
            fields[1] > MAX_EIGHT_BIT_VALUE ||
            fields[2] > MAX_SIXTEEN_BIT_VALUE ) {
            return (INADDR_NONE);
        } //if
        else {
            value = (fields[0] << 24) |
            ((fields[1] & MAX_EIGHT_BIT_VALUE) << 16) |
            (fields[2] & MAX_SIXTEEN_BIT_VALUE);
        } //else
        break;

      case 4:            // a.b.c.d -- 8.8.8.8 bits
        if (fields[0] > MAX_EIGHT_BIT_VALUE ||
            fields[1] > MAX_EIGHT_BIT_VALUE ||
            fields[2] > MAX_EIGHT_BIT_VALUE ||
            fields[3] > MAX_EIGHT_BIT_VALUE ) {
            return (INADDR_NONE);
        } //if
        else {
            value = (fields[0] << 24) |
            ((fields[1] & MAX_EIGHT_BIT_VALUE) << 16) |
            ((fields[2] & MAX_EIGHT_BIT_VALUE) << 8) |
            (fields[3] & MAX_EIGHT_BIT_VALUE);
        } //else
        break;

        // if the address string handed to us has more than 4 address
        // fields return an error
      default:
        return (INADDR_NONE);
    } // switch
    // convert the value to network byte order and return it to the user.
    value = HTONL(value);
    return (value);
}
//--------------------------------------------------------------------
char*
getfilenAmefrompAth(char*	pAth)
{
	int		length;
	int		i;
	char*	filenAme;

	length = strlen(pAth);
	for(i = length;i > 0;i--){
		if(pAth[i] == '\\'){
			filenAme = pAth+i+1;
			return filenAme;
		}
	}
	return NULL;
}
//--------------------------------------------------------------------
int Atoi(char* string)
{
	int		integer = 0;
	int		length;
	int		i;
	int		j;

	length = strlen(string);
	for(i = length;i > 0;i --){
		int temp;
		temp = (int)(string[i-1] - '0');
		for(j = 0;j < length - i;j++){	
			temp *= 10;
		}
		integer += temp;
	}

	return integer;
}
//--------------------------------------------------------------------
int mAx(int A,int b)
{
	return A>b?A:b;
}
//--------------------------------------------------------------------
BOOLEAN
lArge2string(
	LARGE_INTEGER	lArge,
	CHAR*			string,
	ULONG			length
	)
{
	LARGE_INTEGER	Divisor = {10};
	LARGE_INTEGER	RemAinder;
	char			tempstring[50] = {0};
	char			tempstring2[50] = {0};
	ULONG	i = 0,j = 0;
	int		len;


	if(RtlLargeIntegerEqualToZero(lArge)){
		string[0] = '0';
		string[1] = '\0';
		return TRUE;
	}
	//
	//tempstring 把lArge integer转换为字符,倒序
	//
	i = 0;
	while (RtlLargeIntegerGreaterThanZero(lArge)){
		lArge = RtlLargeIntegerDivide(lArge,Divisor,&RemAinder);
		tempstring[i]	= (char)RemAinder.LowPart + '0';
		i++;
	}
	tempstring[i] = '\0';


	if (length < strlen(tempstring) + strlen(tempstring)/3)
		return FALSE;


	//
	//tempstring2 用来加逗号
	//
	j = 0;
	for (i = 0;i < strlen(tempstring);i++){
		tempstring2[j] = tempstring[i];
		if (((i+1) % 3 == 0)&& (i+1) != strlen(tempstring) ){
			j++;
			tempstring2[j] = ',';
		}
		j++;
	}
	tempstring2[j] = '\0';


	for (i = strlen(tempstring2);i > 0;i--){
		string[strlen(tempstring2)-i] = tempstring2[i-1];
	}
	string[strlen(tempstring2)] = '\0';

	return TRUE;
}
//--------------------------------------------------------------------