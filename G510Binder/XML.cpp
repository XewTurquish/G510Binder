//
//  XML.m
//  iCAD
//
//  Created by Josh Anderson on 5/11/11.
//  Copyright 2011 Novatek Inc. All rights reserved.
//

#include "XML.h"
#include <iostream>
#include <fstream>

char *Beggining;
int Total;

XMLAttribute::XMLAttribute() 
{
	Name = "";
	Value = "";
}

XMLAttribute::XMLAttribute(XMLAttribute *Attribute)
{
	Name = Attribute->Name;
	Value = Attribute->Value;
}

XMLAttribute::XMLAttribute(char *&utf8String)
{
	char *Pos = utf8String;
	char *Start;
    
	SkipWhiteSpace(Pos);
	Start = Pos;
	if(!*Pos)
		return;

	GoToChar(Pos, '=');
	if(!*Pos)
		return;
	*Pos = 0;
	Name = string(Start);
	Pos += 2;
	Start = Pos;

	GoToChar(Pos, '\"');
	if(!*Pos)
		return;
	*Pos = 0;
	Value = string(Start);
	utf8String = Pos+1;
 
}

void XMLAttribute::WriteXMLAttrbute(std::ofstream &output)
{
	output << " " << Name << "=\"" << Value << "\"";
}

string XMLAttribute::ToString()
{
	string Return = "";
	Return = Name + "=\"" + Value + "\"";
	
	return Return;
}

XMLNode::XMLNode()
{
	Parent = NULL;
	Name = "";
	Value = "";
}

XMLNode::XMLNode(XMLNode *Node)
{
	if(Node)
	{
		if(Node->Name != "")
			Name = Node->Name;
		if(Node->Value != "")
			Value = Node->Value;
		XMLAttribute *Attribute;
		for(int i = 0; i < Node->Attributes.size(); i++)
		{
			Attribute = new XMLAttribute(Node->Attributes[i]);
			Attributes.push_back(Attribute);
		}
		XMLNode *Current;
		for(int i = 0; i < Node->Children.size(); i++)
		{
			Current = new XMLNode(Node->Children[i]);
			Current->Parent = this;
			Children.push_back(Current);
		}
	}
	else
	{
		Parent = NULL;
		Name = "";
		Value = "";
	}
}

XMLNode::XMLNode(char *&utf8String, XMLNode *NewParent)
{
	char *Pos = utf8String;
	char *Start = Pos;
	XMLAttribute *NewAttribute;

	SkipWhiteSpace(Pos);
	if(*Pos != '<')
		return;
	Parent = NewParent;
	Pos++;
	if(*Pos == '?')
	{
		Pos++;
		Start = Pos;

		GetToWhiteSpace(Pos);
		if(!*Pos)
			return;
		*Pos = 0;
		Name = string(Start);
		Pos++;
		while(*Pos && *Pos != '?')
		{
			NewAttribute = new XMLAttribute(Pos);
			Attributes.push_back(NewAttribute);
			while(IsWhiteSpace(*Pos))
				Pos++;
		}
		Pos += 2;
		utf8String = Pos;
	}
	else if(*Pos == '!')
	{
		while(*Pos)
		{
			if(*Pos == '-')
				if(*(Pos+1) == '-')
					if(*(Pos+2) == '>')
					{
						Pos += 3;
						break;
					}
			Pos++;
		}
		utf8String = Pos;
		return;
	}
	else
	{
		bool IsEndTag = false;
		if(*Pos == '/')
			IsEndTag = true;
		Start = Pos;

		GetToWhiteSpace(Pos, '>');
		if(!*Pos)
			return;
		bool CloseTag = false;
		if(*Pos == '>')
			CloseTag = true;
		*Pos = 0;
		Name = string(Start);
		if(IsEndTag)
		{
			utf8String = Pos+1;
			return;
		}
		Pos++;
		while(*Pos && !CloseTag && *Pos != '>' && *Pos != '/')
		{
			NewAttribute = new XMLAttribute(Pos);
			Attributes.push_back(NewAttribute);
			SkipWhiteSpace(Pos);
		}
		if(!*Pos)
			return;
		if(*Pos == '/')
		{
			utf8String = Pos+2; 
			return;
		}
		if(!CloseTag)
			Pos++;

		SkipWhiteSpace(Pos);
		if(!*Pos)
			return;
		if(*Pos != '<')
		{
			Start = Pos;

			GoToChar(Pos, '<');
			if(!*Pos)
				return;
			*Pos = 0;
			Value = string(Start);
			*Pos = '<';
		}
		XMLNode *CurrentNode = new XMLNode(Pos, this);
		while(CurrentNode && CurrentNode->Name != "" && (string("/") + Name) != CurrentNode->Name)
		{
			Children.push_back(CurrentNode);
			CurrentNode = new XMLNode(Pos, this);
		}
		if(CurrentNode)
			delete CurrentNode;
	}
	SkipWhiteSpace(Pos);
	utf8String = Pos;
}

XMLNode::~XMLNode()
{
	for(int i = 0 ; i < Children.size(); i++)
		delete Children[i];
	for(int i = 0; i < Attributes.size(); i++)
		delete Attributes[i];
}

void XMLNode::WriteXMLNode(std::ofstream &output)
{
	int Depth = GetNodeDepth();
	for(int i = 0; i < Depth; i++)
		output << "\t";
    
	output << "<";
	if(Name == "xml")
		output << "?";
	
	output << Name;
	for(int i = 0; i < Attributes.size(); i++)
		Attributes[i]->WriteXMLAttrbute(output);
	if(Value != "" || Children.size())
	{
		if(Name == "xml")
			output << "?";
		output << ">";
		if(Value != "")
			output << Value;
		if(Children.size())
		{
			for(int i = 0; i < Children.size(); i++)
			{
				output << "\n";
				Children[i]->WriteXMLNode(output);
			}
			output << "\n";
			for(int i = 0; i < Depth; i++)
				output << "\t";
	
			output << "</";
			output << Name;
			output << ">";
		}
		else
		{
			output << "</";
			output << Name;
			output << ">";
		}
	}
	else
	{
		if(Name == "xml")
			output << "?";
		else
			output << "/";
		output << ">";
	}
}

int XMLNode::GetNodeDepth()
{
	int Return = 0;
	if(Parent)
		Return += 1 + Parent->GetNodeDepth();
	return Return;
}

std::vector<XMLNode *> XMLNode::GetXMLNodes(string gName, string AName, string AValue)
{
	std::vector<XMLNode *>Nodes;
	if(gName ==Name)
	{
		if(AName != "")
		{
			int i;
			for(i = 0; i < Attributes.size(); i++)
			{
				if(ToLowerString(Attributes[i]->Name) == ToLowerString(AName))
					break;
			}
			if(i < Attributes.size())
			{
				if(AValue != "")
				{
					if(ToLowerString(AValue) == ToLowerString(Attributes[i]->Value))
						Nodes.push_back(this);
				}
				else
					Nodes.push_back(this);
			}
		}
		else
			Nodes.push_back(this);
	}
	std::vector<XMLNode *>Temp;
	for(int i = 0; i < Children.size(); i++)
	{
		Temp = Children[i]->GetXMLNodes(gName, AName, AValue);
		for(int j = 0; j < Temp.size(); j++)
			Nodes.push_back(Temp[j]);
	}
	return Nodes;
}

XMLNode *XMLNode::GetXMLNode(string gName, string AName, string AValue)
{
	if(gName == "")
	{
		for(int i = 0; i < Attributes.size(); i++)
		{
			if(ToLowerString(Attributes[i]->Name) == ToLowerString(AName))    
			{
				if(AValue != "")
				{
					if(ToLowerString(Attributes[i]->Value) == ToLowerString(AValue))
						return this;
				}
				else
					return this;
			}
		}
		for(int i = 0; i < Children.size(); i++)
		{
			XMLNode *Return = Children[i]->GetXMLNode(gName, AName, AValue);
			if(Return != NULL)
				return Return;
		}
	}
	else
	{
		if(gName == Name)
		{
			if(AName != "")
			{
				for(int i = 0; i < Attributes.size(); i++)
				{
					if(ToLowerString(Attributes[i]->Name) == ToLowerString(AName))
					{
						if(AValue != "")
						{
							if(ToLowerString(Attributes[i]->Value) == ToLowerString(AValue))
								return this;
						}
						else
							return this;
					}
				}
			}
			else
				return this;
		}
		for(int i = 0; i < Children.size(); i++)
		{
			XMLNode *Return = Children[i]->GetXMLNode(gName, AName, AValue);
			if(Return != NULL)
				return Return;
		}
	}
    
	return NULL;
}

std::vector<XMLNode *> XMLNode::GetXMLNodesWithValue(string Value, string nName, string AName, string AValue)
{
	std::vector<XMLNode *> Return = GetXMLNodes(nName, AName, AValue);

	for(int i = 0; i < Return.size(); i++)
	{
		if(Return[i]->Value != Value)
		{
			Return.erase(Return.begin()+i,Return.begin()+i);
			i--;
		}
	}

	return Return;
}

XMLNode * XMLNode::GetXMLNodeWithValue(string Value, string nName, string AName, string AValue)
{
	std::vector<XMLNode *> List = GetXMLNodes(nName, AName, AValue);
	for(int i = 0; i < List.size(); i++)
		if(List[i]->Value == Value)
			return List[i];

	return NULL;
}

XMLAttribute *XMLNode::GetAttribute(string AName)
{
	for(int i = 0; i < Attributes.size(); i++)
	{
		if(ToLowerString(Attributes[i]->Name) == ToLowerString(AName))
			return Attributes[i];
	}
	return NULL;
}

string XMLNode::GetAttributeValue(string AName)
{
	for(int i = 0; i < Attributes.size(); i++)
	{
		if(ToLowerString(Attributes[i]->Name) == ToLowerString(AName))
			return Attributes[i]->Value;
	}
	return NULL;
}

string XMLNode::ToString()
{
	string Return = "";
	int Depth = GetNodeDepth();
	for(int i = 0; i < Depth; i++)
		Return += "\t";
	Return += "<";
	if(Name == "xml")
		Return += "?";
	Return += Name;
	for(int i = 0; i < Attributes.size(); i++)
		Return += " " + Attributes[i]->ToString();
	if(Value != "" || Children.size())
	{
		Return += ">";
		if(Value != "")
			Return += Value;
		for(int i = 0; i < Children.size(); i++)
			Return += "\n" + Children[i]->ToString();
		if(Children.size())
			Return += "\n";
		for(int i = 0; i < Depth; i++)
			Return += "\t";
		Return += "</" + Name + ">";
	}
	else
	{
		if(Name == "xml")
			Return += "?>";
		else
			Return += "/>";
	}
    
	return Return;
}

string XMLNode::ToString(string *String, bool isRoot)
{
	XMLNode *xmlParent = Parent;
	if(isRoot)
		Parent = NULL;
	string Return = ToString();
	Parent = xmlParent;
    
	if(String != NULL)
		*String = Return;
    
	return Return;
}

unsigned int XMLNode::Count()
{
	return Children.size();
}

XML::XML()
{
}

XML::XML(std::vector<XMLNode *> Nodes)
{
	XMLNode *Current;
	for(int i = 0; i < Nodes.size(); i++)
	{
		Current = new XMLNode(Nodes[i]);
		Roots.push_back(Current);
	}
}

XML::~XML()
{
	for(int i = 0; i < Roots.size(); i++)
		delete Roots[i];
}

void XML::LoadXMLFile(string filePath)
{
	std::ifstream input;
	input.open(filePath.c_str());
	if(!input.is_open())
		return;
	input.seekg(0,std::ios::end);
	unsigned long long Size = input.tellg();
	input.close();
	input.open(filePath.c_str());
	char *Buffer = new char[Size+1];
	Beggining = Buffer;
	input.read(Buffer, Size);
	Buffer[Size] = 0;
	input.close();
	char *Pos = Buffer;
	XMLNode *xmlVerison = new XMLNode(Pos, NULL);
	if(xmlVerison && xmlVerison->Name != "")
		Roots.push_back(xmlVerison);
	XMLNode *CurrentNode;
	CurrentNode = new XMLNode(Pos, NULL);
	if(CurrentNode && CurrentNode->Name != "")
		Roots.push_back(CurrentNode);
	delete[] Buffer;
}

void XML::WriteXMLFile(string filePath)
{
	std::ofstream output;
	output.open(filePath.c_str());
	for(int i = 0; i < Roots.size(); i++)
	{
		Roots[i]->WriteXMLNode(output);
		if(i < Roots.size()-1)
			output << "\n";
	}
	output.close();
}

std::vector<XMLNode *> XML::GetXMLNodes(string Name, string AName, string Value)
{
	std::vector<XMLNode *>Nodes;
	std::vector<XMLNode *>Temp;
	for(int i = 0; i < Roots.size(); i++)
	{
		Temp = Roots[i]->GetXMLNodes(Name, AName, Value);
		for(int j = 0; j < Temp.size(); j++)
			Nodes.push_back(Temp[j]);
	}
    
	return Nodes;
}

XMLNode *XML::GetXMLNode(string Name, string AName, string Value)
{
	XMLNode *Return = NULL;
	for(int i = 0; i < Roots.size(); i++)
	{
		Return = Roots[i]->GetXMLNode(Name, AName, Value);
		if(Return != NULL)
			return Return;
	}
	return NULL;
}

unsigned int XML::Count()
{
	return Roots.size();
}

void SkipWhiteSpace(char *&Pos)
{
	while(*Pos && IsWhiteSpace(*Pos))
		Pos++;
}

void GetToWhiteSpace(char *&Pos, char Delim)
{
	while(*Pos && !IsWhiteSpace(*Pos) && *Pos != Delim)
		Pos++;
}

void GoToChar(char *&Pos, char Delim)
{
	while(*Pos && *Pos != Delim)
		Pos++;
}

bool IsWhiteSpace(char Pos)
{
	if(Pos == ' ' || Pos == '\n' || Pos == '\r' || Pos == '\t')
		return true;

	return false;
}

string ToLowerString(string &Source)
{	
	return string(strlwr((char*)Source.c_str()));
}

int _httoi(const TCHAR *value)
{
  struct CHexMap
  {
    TCHAR chr;
    int value;
  };
  const int HexMapL = 16;
  CHexMap HexMap[HexMapL] =
  {
    {'0', 0}, {'1', 1},
    {'2', 2}, {'3', 3},
    {'4', 4}, {'5', 5},
    {'6', 6}, {'7', 7},
    {'8', 8}, {'9', 9},
    {'A', 10}, {'B', 11},
    {'C', 12}, {'D', 13},
    {'E', 14}, {'F', 15}
  };
  TCHAR *mstr = _tcsupr(_tcsdup(value));
  TCHAR *s = mstr;
  int result = 0;
  if (*s == '0' && *(s + 1) == 'X') s += 2;
  bool firsttime = true;
  while (*s != '\0')
  {
    bool found = false;
    for (int i = 0; i < HexMapL; i++)
    {
      if (*s == HexMap[i].chr)
      {
        if (!firsttime) result <<= 4;
        result |= HexMap[i].value;
        found = true;
        break;
      }
    }
    if (!found) break;
    s++;
    firsttime = false;
  }
  free(mstr);
  return result;
}