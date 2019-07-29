#include "xml.hpp"
#include "Logs/logs.hpp"
#include <fstream>

namespace ph {

void Xml::loadFromFile(const std::string& filePath)
{
	mContent.clear();
	std::string fullFilePath = "resources/" + filePath;
	std::ifstream ifs(fullFilePath);
	if (!ifs.is_open())
		PH_EXCEPTION("cannot open file: " + fullFilePath);
	std::string temp;
	if (!std::getline(ifs, temp))
		PH_EXCEPTION("given xml file is empty or something bad happened (" + fullFilePath + ")");
	// NOTE: Delete prolog but keep '?>' for implementation purpose
	const std::size_t begin = temp.find("?>");
	if (begin == std::string::npos)
		temp.insert(0, "?>");
	else
		temp.erase(0, begin);
	mContent += temp;
	while (std::getline(ifs, temp))
		mContent += temp;
	PH_LOG(LogLevel::Info, std::string("Xml loadFromFile(): ") + mContent);
}

Xml Xml::getChild(std::string name) const
{
	PH_ASSERT(!name.empty(), "child name cannot be empty");
	std::size_t begin = findEndOfTagAttributes();
	PH_ASSERT(begin != std::string::npos, "missing closing angle bracket");
	if (isSelfClosingTag(begin))
		PH_EXCEPTION("current tag cannot have children");
	while (true) {
		begin = mContent.find('<', begin + 1);
		if (begin == std::string::npos)
			PH_EXCEPTION("cannot find child");
		++begin;
		std::size_t end = mContent.find_first_of(whitespaceCharacters + ">", begin + 1);
		if (end == std::string::npos)
			PH_EXCEPTION("missing angle bracket in child opening tag");
		if (isSelfClosingTag(end)) {
			if (mContent.compare(begin, end - begin - 1, name) == 0) {
				Xml xml;
				xml.mContent = mContent.substr(begin, end - begin + 1);
				PH_LOG(LogLevel::Info, "Xml getChild(): " + xml.mContent);
				return xml;
			}
			else
				begin = end;
		}
		else if (mContent.compare(begin, end - begin, name) == 0) {
			if (mContent[end] != '>') {
				end = findEndOfTagAttributes(end + 1);
				if (end == std::string::npos)
					PH_EXCEPTION("missing closing angle bracket in child opening tag");
			}
			if (isSelfClosingTag(end)) {
				Xml xml;
				xml.mContent = mContent.substr(begin, end - begin + 1);
				PH_LOG(LogLevel::Info, "Xml getChild(): " + xml.mContent);
				return xml;
			}
			unsigned count = 0;
			while (true) {
				end = mContent.find(name, end + 2);
				if (end == std::string::npos)
					PH_EXCEPTION("missing child closing tag");
				if (isClosingTag(end) && count == 0) {
					Xml xml;
					xml.mContent = mContent.substr(begin, end - begin - 2);
					PH_LOG(LogLevel::Info, "Xml getChild(): " + xml.mContent);
					return xml;
				}
				else if (isClosingTag(end)) {
					--count;
					end += name.size();
				}
				else if (mContent[end - 1] == '<') {
					end = findEndOfTagAttributes(end + name.size());
					if (end == std::string::npos)
						PH_EXCEPTION("missing closing angle bracket in child opening tag");
					if (!isSelfClosingTag(end))
						++count;
				}
				else
					end += name.size() - 1;
			}
		}
		else {
			/* 
				NOTE:
				It is child which is not equel to given name paramiter.
				So we need to find his ending tag to skip him and his children.
			*/
			const std::string childName = mContent.substr(begin, end - begin);
			begin = end;
			if (mContent[begin] != '>') {
				begin = findEndOfTagAttributes(begin + 1);
				if (begin == std::string::npos)
					PH_EXCEPTION("missing closing angle bracket in child opening tag");
			}
			if (isSelfClosingTag(begin))
				continue;
			unsigned count = 0;
			while (true) {
				begin = mContent.find(childName, begin + 2);
				if (begin == std::string::npos)
					PH_EXCEPTION("missing child closing tag");
				if (isClosingTag(begin) && count == 0) {
					begin += childName.size();
					break;
				}
				else if (isClosingTag(begin)) {
					--count;
					begin += childName.size();
				}
				else if (mContent[begin - 1] == '<') {
					begin = findEndOfTagAttributes(begin + childName.size());
					if (begin == std::string::npos)
						PH_EXCEPTION("missing closing angle bracket in child opening tag");
					if (!isSelfClosingTag(begin))
						++count;
				}
				else
					begin += childName.size() - 1;
			}
		}
	}
}

std::vector<Xml> Xml::getChildren(std::string name) const
{
	PH_ASSERT(!name.empty(), "child name cannot be empty");
	std::size_t begin = findEndOfTagAttributes();
	PH_ASSERT(begin != std::string::npos, "missing closing angle bracket");
	if (isSelfClosingTag(begin))
		return std::vector<Xml>();
	std::vector<Xml> children;
	while ((begin = mContent.find('<', begin + 1)) != std::string::npos) {
		++begin;
		std::size_t end = mContent.find_first_of(whitespaceCharacters + " >", begin + 1);
		if (end == std::string::npos)
			PH_EXCEPTION("missing angle bracket in child opening tag");
		if (isSelfClosingTag(end)) {
			if (mContent.compare(begin, end - begin - 1, name) == 0) {
				Xml xml;
				xml.mContent = mContent.substr(begin, end - begin + 1);
				PH_LOG(LogLevel::Info, "Xml getChildren(): " + xml.mContent);
				children.push_back(xml);
				begin = end;
				continue;
			}
			else
				begin = end;
		}
		else if (mContent.compare(begin, end - begin, name) == 0) {
			if (mContent[end] != '>') {
				end = findEndOfTagAttributes(end + 1);
				if (end == std::string::npos)
					PH_EXCEPTION("missing closing angle bracket in child opening tag");
				if (isSelfClosingTag(end)) {
					Xml xml;
					xml.mContent = mContent.substr(begin, end - begin + 1);
					PH_LOG(LogLevel::Info, "Xml getChildren(): " + xml.mContent);
					children.push_back(xml);
					begin = end;
					continue;
				}
			}
			unsigned count = 0;
			while (true) {
				end = mContent.find(name, end + 2);
				if (end == std::string::npos)
					PH_EXCEPTION("missing child closing tag");
				if (isClosingTag(end) && count == 0) {
					Xml xml;
					xml.mContent = mContent.substr(begin, end - begin - 2);
					PH_LOG(LogLevel::Info, "Xml getChildren(): " + xml.mContent);
					children.push_back(xml);
					begin = end + name.size();
					break;
				}
				else if (isClosingTag(end)) {
					--count;
					end += name.size();
				}
				else if (mContent[end - 1] == '<') {
					end = findEndOfTagAttributes(end + name.size());
					if (end == std::string::npos)
						PH_EXCEPTION("missing closing angle bracket in child opening tag");
					if (!isSelfClosingTag(end))
						++count;
				}
				else
					end += name.size() - 1;
			}
		}
		else {
			/*
				NOTE:
				It is child which is not equel to given name paramiter.
				So we need to find his ending tag to skip him and his children.
			*/
			const std::string childName = mContent.substr(begin, end - begin);
			begin = end;
			if (mContent[begin] != '>') {
				begin = findEndOfTagAttributes(begin + 1);
				if (begin == std::string::npos)
					PH_EXCEPTION("missing closing angle bracket in child opening tag");
				if (isSelfClosingTag(begin))
					continue;
			}
			unsigned count = 0;
			while (true) {
				begin = mContent.find(childName, begin + 2);
				if (begin == std::string::npos)
					PH_EXCEPTION("missing child closing tag");
				if (isClosingTag(begin) && count == 0) {
					begin += childName.size();
					break;
				}
				else if (isClosingTag(begin)) {
					--count;
					begin += childName.size();
				}
				else if (mContent[begin - 1] == '<') {
					begin = findEndOfTagAttributes(begin + childName.size());
					if (begin == std::string::npos)
						PH_EXCEPTION("missing closing angle bracket in child opening tag");
					if (!isSelfClosingTag(begin))
						++count;
				}
				else
					begin += childName.size() - 1;
			}
		}
	}
	return children;
}

bool Xml::hasAttribute(std::string name) const
{
	PH_ASSERT(!name.empty(), "attribute name cannot be empty");
	/*
		NOTE:
		This partly protects against situation when attribute name will
		be in part of other attribute name or in attribute value

		WARNING: It fails with spaces: <foo bar  =    ""
		TODO: Delete unnecessary spaces in loadFromFile()?
	*/
	name.insert(0, " ");
	name.push_back('=');
	name.push_back('\"');

	std::size_t begin = mContent.find(name);
	if (begin == std::string::npos) {
		PH_LOG(LogLevel::Info, "Xml hasAttribute(): false");
		return false;
	}
	begin += name.size();

	std::size_t end = mContent.find('>');
	if (end == std::string::npos)
		PH_EXCEPTION("missing angle bracket in child opening tag");
	if (isSelfClosingTag(end))
		--end;
	if (begin >= end) {
		PH_LOG(LogLevel::Info, "Xml hasAttribute(): false");
		return false;
	}
	if (isEmptyAttributeValue(begin)) {
		PH_LOG(LogLevel::Info, "Xml hasAttribute(): true");
		return true;
	}

	end = mContent.find('\"', begin + 1);
	if (end == std::string::npos)
		PH_EXCEPTION("missing closing quote");

	PH_LOG(LogLevel::Info, "Xml hasAttribute(): true");
	return true;
}

Xml Xml::getAttribute(std::string name) const
{
	PH_ASSERT(!name.empty(), "attribute name cannot be empty");
	/*
		NOTE:
		This partly protects against situation when attribute name will
		be in part of other attribute name or in attribute value

		WARNING: It fails with spaces: <foo bar  =    ""
		TODO: Delete unnecessary spaces in loadFromFile()?
	*/
	name.insert(0, " ");
	name.push_back('=');
	name.push_back('\"');

	std::size_t begin = mContent.find(name);
	if (begin == std::string::npos)
		PH_EXCEPTION("attribute name cannot be found");
	begin += name.size();

	std::size_t end = mContent.find('>');
	if (end == std::string::npos)
		PH_EXCEPTION("missing angle bracket in child opening tag");
	if (isSelfClosingTag(end))
		--end;
	if (begin >= end)
		PH_EXCEPTION("attribute name cannot be found");
	if (isEmptyAttributeValue(begin))
		return Xml();

	end = mContent.find('\"', begin + 1);
	if (end == std::string::npos)
		PH_EXCEPTION("missing closing quote");

	Xml xml;
	xml.mContent = mContent.substr(begin, end - begin);
	PH_LOG(LogLevel::Info, "Xml getAttribute(): " + xml.mContent);
	return xml;
}

std::string Xml::toString() const
{
	std::size_t begin = findEndOfTagAttributes();
	if (begin == std::string::npos)
		return mContent;
	++begin;
	return mContent.substr(begin, mContent.size() - begin);
}

}