//
//  XML.h
//  iCAD
//
//  Created by Josh Anderson on 5/11/11.
//  Copyright 2011 Novatek Inc. All rights reserved.
//

#include <vector>
#include <fstream>
#include <string>
#include <tchar.h>

using namespace std;

class XMLAttribute
{
public:
	XMLAttribute();
	XMLAttribute(XMLAttribute *Source);
	XMLAttribute(char *&utf8String);
	void WriteXMLAttrbute(std::ofstream &output);
	string ToString();

	string Name;
	string Value;
};

class XMLNode
{
public:
	XMLNode();
	XMLNode(XMLNode *Node);
	XMLNode(char *&utf8String, XMLNode *Parent);
	~XMLNode();

	void	WriteXMLNode(std::ofstream &output);
	string ToString();
	string ToString(string *String, bool isRoot);
	int GetNodeDepth();
	std::vector<XMLNode *> GetXMLNodes(string gName, string AName = "", string AValue = "");
	XMLNode *GetXMLNode(string gName, string AName = "", string AValue = "");
	std::vector<XMLNode *> GetXMLNodesWithValue(string Value, string nName = "", string AName = "", string AValue = "");
	XMLNode *GetXMLNodeWithValue(string Value, string nName = "", string AName = "", string AValue = "");
	XMLAttribute *GetAttribute(string AName);
	string GetAttributeValue(string AName);
	unsigned int Count();

	XMLNode *Parent;
	std::vector<XMLNode *> Children;
	std::vector<XMLAttribute *> Attributes;
	string Value;
	string Name;
};

class XML
{
public:
	XML();
	XML(std::vector<XMLNode *> Nodes);
	~XML();
	
	void LoadXMLFile(string filePath);
	void WriteXMLFile(string filePath);
	std::vector<XMLNode *> GetXMLNodes(string Name, string AName= "", string Value = "");
	XMLNode *GetXMLNode(string Name, string AName = "", string Value = "");
	unsigned int Count();

	std::vector<XMLNode *> Roots;
};

void SkipWhiteSpace(char *&Pos);
void GetToWhiteSpace(char *&Pos, char Delim = 0);
void GoToChar(char*&Pos, char Delim);
bool IsWhiteSpace(char Pos);
string ToLowerString(string &Source);
int _httoi(const TCHAR *value);