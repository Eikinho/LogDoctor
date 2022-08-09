#ifndef STRINGS_H
#define STRINGS_H

#include <string>
#include <vector>


class StringOps
{
public:
    StringOps();


    static const int count( const std::string& str, const std::string& flag, const bool& consecutives=true );

    static const bool isNumeric( const std::string& str );
    static const bool isNumeric( const char& chr );

    static const bool isAlphabetic( const std::string& str );
    static const bool isAlphabetic( const char& chr );

    static const bool isAlnum( const std::string& str );
    static const bool isAlnum( const char& chr );

    static const bool startsWith( const std::string& str, const std::string& flag );
    static const bool endsWith(   const std::string& str, const std::string& flag );

    static const bool contains( const std::string& str, const std::string& flag );

    static std::string strip(  const std::string& str, const std::string& chars=" \n\t\b\r\v" );
    static std::string lstrip( const std::string& str, const std::string& chars=" \n\t\b\r\v" );
    static std::string rstrip( const std::string& str, const std::string& chars=" \n\t\b\r\v" );
    static std::string lstripUntil( const std::string& str, const std::string& chr, const bool& inclusive=true, const bool& consecutives=true );

    static void split( std::vector<std::string>& list, const std::string& target_str, const std::string& separator="\n" );

    static void splitrip( std::vector<std::string>& list, const std::string& target_str, const std::string& separator="\n", const std::string& strip=" \n\t\b\r\v" );

    static std::string replace( const std::string& str, const std::string& target, const std::string& replace );
};

#endif // STRINGS_H
