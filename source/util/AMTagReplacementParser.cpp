#include "AMTagReplacementParser.h"
#include "util/AMErrorMonitor.h"

#include <QDebug>

AMTagReplacementParser::AMTagReplacementParser(const QChar& tagStartCharacter, const QChar& argumentStartCharacter, const QChar& argumentEndCharacter)
{
	tagStart_ = tagStartCharacter;
	argumentStart_ = argumentStartCharacter;
	argumentEnd_ = argumentEndCharacter;
}

void AMTagReplacementParser::setInitialText(const QString &initialText)
{
	initialText_ = initialText;
	strippedText_.clear();
	replacementList_.clear();

	int posIEnd = initialText_.size();
	strippedText_.reserve(posIEnd);

	parseState_ = NormalText;

	// go through the string (one pass, character by character)
	for(int posI = 0; posI < posIEnd; posI++) {

		QChar c = initialText_.at(posI);

		switch(parseState_) {

		case InsideTag:
			if(c == argumentStart_) {
				// qDebug() << "starting argument" << c;
				startArgument();
			}
			else if(c == tagStart_) {
				// exception: if this is the second of two start tags in a row: is an "escaped start tag"... Put one start tag into the output stream and go back to normal parsing.
				if(currentTag_.tag.size() == 0) {
					// qDebug() << "escaped start char" << c;
					strippedText_.append(c);
					parseState_ = NormalText;
				}
				else {	// otherwise this is just the end of one tag and the start of the next.
					// qDebug() << "Ending AND starting tag!" << c;
					endTag();
					startTag();
				}
			}
			else if(c.isLetterOrNumber()) {
				// qDebug() << "Adding to tag text:" << c;
				currentTag_.tag.append(c);
			}
			else {
				// qDebug() << "ending tag at non-letterOrNumber" << c;
				endTag();
				strippedText_.append(c);
			}
			break;



		case InsideArg:
			if(c == argumentEnd_) {
				// qDebug() << "ending argument" << c;
				endArgument();
			}
			else {
				// qDebug() << "Adding to arg text:" << c;
				currentTag_.arguments.append(c);
			}
			break;



		case NormalText:
			if(c == tagStart_) {
				// qDebug() << "Entering tag" << c;
				startTag();
			}
			else {
				// qDebug() << "Adding to normal text:" << c;
				strippedText_.append(c);
			}
			break;
		}
	}

	if(parseState_ == InsideTag)
		endTag();

	/// \todo What to do with dangling arguments at end of text?
	if(parseState_ == InsideArg)
		AMErrorMon::report(AMErrorReport(0, AMErrorReport::Alert, -1,
										 QString("Warning: dangling argument when parsing tag '%1' within some replacement text.").arg(currentTag_.tag)));
}

QString AMTagReplacementParser::getReplacedText()
{
	QString rv;
	// performance optimization: estimating every replacement will be around 40 chars. Reserve enough size for whole output string at once.
	rv.reserve(strippedText_.size() + replacementList_.count()*40);

	int strippedTextPosition = 0;

	for(int i=0; i<replacementList_.count(); i++) {
		const AMTagReplacement& currentTag = replacementList_.at(i);

		while(strippedTextPosition < currentTag.position)
			rv.append(strippedText_.at(strippedTextPosition++));

		rv.append(currentTag.replacement);
	}

	// put on last of text after last tag
	rv.append(strippedText_.mid(strippedTextPosition));

	return rv;
}

void AMTagReplacementParser::startArgument()
{
	parseState_ = InsideArg;
}

void AMTagReplacementParser::endArgument()
{
	endTag();
}

void AMTagReplacementParser::startTag()
{
	currentTag_.tag.clear();
	currentTag_.arguments.clear();
	currentTag_.position = strippedText_.size();
	parseState_ = InsideTag;
}

void AMTagReplacementParser::endTag()
{
	replacementList_ << currentTag_;
	parseState_ = NormalText;
}



void AMTagReplacementParser::replaceAllUsingDictionary(const QHash<QString, AMAbstractTagReplacementFunctor *> &lookupDictionary) {
	// loop through all replacement items
	for(int i=replacementList_.count()-1; i>=0; i--) {
		QHash<QString, AMAbstractTagReplacementFunctor*>::const_iterator iFunctor = lookupDictionary.find(replacementList_.at(i).tag);

		if(iFunctor != lookupDictionary.end()) {	// if there is a functor for this tag: use it.
			replacementList_[i].replacement = iFunctor.value()->getText(replacementList_.at(i).arguments);
		}
		else {	// otherwise we have no idea what should go here
			replacementList_[i].replacement = "[?]";
		}
	}
}