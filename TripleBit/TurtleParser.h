#ifndef H_tools_rdf3xload_TurtleParser
#define H_tools_rdf3xload_TurtleParser

//---------------------------------------------------------------------------
// TripleBit
// (c) 2011 Massive Data Management Group @ SCTS & CGCL. 
//     Web site: http://grid.hust.edu.cn/triplebit
//
// This work is licensed under the Creative Commons
// Attribution-Noncommercial-Share Alike 3.0 Unported License. To view a copy
// of this license, visit http://creativecommons.org/licenses/by-nc-sa/3.0/
// or send a letter to Creative Commons, 171 Second Street, Suite 300,
// San Francisco, California, 94105, USA.
//---------------------------------------------------------------------------

#include <istream>
#include <string>
#include <map>
#include <vector>
#include "Type.hpp"
//---------------------------------------------------------------------------
/// Parse a turtle file
class TurtleParser
{
   public:
   /// A parse error
   class Exception {};

   private:
   /// A turtle lexer
   class Lexer {//语法分析程序类
      public:
      /// Possible tokens
      enum Token { Eof, Dot, Colon, Comma, Semicolon, LBracket, RBracket, LParen, RParen, At, Type, Integer, Decimal, Double, Name, A, True, False, String, URI };
      	  	  	  //结束，点号，冒号，  逗号，  分号，      左括号     右括号、    左括弧  右括弧   @
      private:
      /// The input
      std::istream& in;
      /// The putback
      Token putBack;
      /// The putback string
      std::string putBackValue;
      /// Buffer for parsing when ignoring the value
      std::string ignored;
      /// The current line
      unsigned line;

      /// Size of the read buffer
      static const unsigned readBufferSize = 1024;
      /// Read buffer
      char readBuffer[readBufferSize];
      /// Read buffer pointers
      char* readBufferStart,*readBufferEnd;

      /// Read new characters
      bool doRead(char& c);
      /// Read a character
      bool read(char& c) { if (readBufferStart<readBufferEnd) { c=*(readBufferStart++); return true; } else return doRead(c); }
      /// Unread the last character
      void unread() { readBufferStart--; }

      /// Lex a hex code
      unsigned lexHexCode(unsigned len);
      /// Lex an escape sequence
      void lexEscape(std::string& token);
      /// Lex a long string
      Token lexLongString(std::string& token);
      /// Lex a string
      Token lexString(std::string& token,char c);
      /// Lex a URI
      Token lexURI(std::string& token,char c);
      /// Lex a number
      Token lexNumber(std::string& token,char c);

      public:
      /// Constructor
      Lexer(std::istream& in);
      /// Destructor
      ~Lexer();

      /// The next token (including value)
      Token next(std::string& value);
      /// The next token (ignoring the value)
      Token next() { return next(ignored); }
      /// Put a token and a string back
      void unget(Token t,const std::string& s) { putBack=t; if (t>=Integer) putBackValue=s; }
      /// Put a token back
      void ungetIgnored(Token t) { putBack=t; if (t>=Integer) putBackValue=ignored; }
      /// Get the line
      unsigned getLine() const { return line; }
   };
   /// A triple
   struct Triple {
      /// The entries
      std::string subject,predicate,object;
      Type::ID objectType;
      /// Constructor
      Triple(const std::string& subject,const std::string& predicate,const std::string& object,Type::ID objectType) : subject(subject),predicate(predicate),object(object),objectType(objectType) {}
   };

   /// The lexer
   Lexer lexer;
   /// The uri base
   std::string base;
   /// All known prefixes
   std::map<std::string,std::string> prefixes;
   /// The currently available triples
   std::vector<Triple> triples;
   /// Reader in the triples
   unsigned triplesReader;
   /// The next blank node id
   unsigned nextBlank;

   /// Is a (generalized) name token?
   static inline bool isName(Lexer::Token token);

   /// Construct a new blank node
   void newBlankNode(std::string& node);
   /// Report an error
   void parseError(const std::string& message);
   /// Parse a qualified name
   void parseQualifiedName(const std::string& prefix,std::string& name);
   /// Parse a blank entry
   void parseBlank(std::string& entry);
   /// Parse a subject
   void parseSubject(Lexer::Token token,std::string& subject);
   /// Parse an object
  // void parseObject(std::string& object);
   void parseObject(std::string& object,Type::ID& objectType);
   /// Parse a predicate object list
   //void parsePredicateObjectList(const std::string& subject,std::string& predicate,std::string& object);

   void parsePredicateObjectList(const std::string& subject,std::string& predicate,std::string& object, Type::ID& objectType);

   /// Parse a directive
   void parseDirective();
   /// Parse a new triple
  // void parseTriple(Lexer::Token token,std::string& subject,std::string& predicate,std::string& object );
   void parseTriple(Lexer::Token token,std::string& subject,std::string& predicate,std::string& object,Type::ID& objectType );
   public:
   /// Constructor
   TurtleParser(std::istream& in);
   /// Destructor
   ~TurtleParser();

   //Read the next triple
   bool parse(std::string& subject,std::string& predicate,std::string& object,Type::ID& objectType);

};
//---------------------------------------------------------------------------
#endif
