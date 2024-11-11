/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.0.4
 Written in Qt
 
                      WebCrawler.cpp  -  description
                             -------------------
    copyright         : (C) 2005-2021 by Dimitris B. Kalamaras
    project site      : https://socnetv.org

 ***************************************************************************/

/*******************************************************************************
*     This program is free software: you can redistribute it and/or modify     *
*     it under the terms of the GNU General Public License as published by     *
*     the Free Software Foundation, either version 3 of the License, or        *
*     (at your option) any later version.                                      *
*                                                                              *
*     This program is distributed in the hope that it will be useful,          *
*     but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
*     GNU General Public License for more details.                             *
*                                                                              *
*     You should have received a copy of the GNU General Public License        *
*     along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
********************************************************************************/

#include "webcrawler.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QLoggingCategory>
#include <QUrl>
#include <QThread>
#include <QtConcurrent/QtConcurrent>
#include <QDeadlineTimer>
#include <QRegularExpression>

#include <functional>
#include <set>

static QLoggingCategory DBG_CATEGORY("net.crawler");

#define qDebug() qCDebug(DBG_CATEGORY)

/**
 * @brief Constructor from parent Graph thread. Inits variables.
 * @param url
 * @param maxNc
 * @param maxLinksPerPage
 * @param extLinks
 * @param intLinks
 */
WebCrawler::WebCrawler(QCQueue<QUrl> *urlQueue,
        const QUrl &startUrl,
        const QStringList &urlPatternsIncluded,
        const QStringList &urlPatternsExcluded,
        const QStringList &linkClasses,
        const int &maxN,
        const int &maxLinksPerPage,
        const bool &intLinks,
        const bool &childLinks,
        const bool &parentLinks,
        const bool &selfLinks,
        const bool &extLinksIncluded,
        const bool &extLinksCrawl,
        const bool &socialLinks,
        const int &delayBetween)
	:
    // Initialize user-defined control variables and limits
	    m_initialUrl(startUrl)
		,m_maxUrls(maxN)								// max urls we'll check == max nodes in the social network
		,m_maxLinksPerPage(maxLinksPerPage)			// max links per page to search for

		,m_intLinks(intLinks)
		,m_childLinks(childLinks)
		,m_parentLinks(parentLinks)
		,m_selfLinks(selfLinks)

		,m_extLinksIncluded(extLinksIncluded)
		,m_extLinksCrawl(extLinksCrawl)
	    ,m_socialLinks(socialLinks)

		,m_delayBetween(delayBetween)

		,m_urlPatternsIncluded(urlPatternsIncluded)		// list of url patterns to include
		,m_urlPatternsExcluded(urlPatternsExcluded)		// list of url patterns to exclude
		,m_linkClasses(linkClasses)						// list of link classes to include
		,m_socialLinksExcluded {
			"facebook.com"
			,"twitter.com"
			,"linkedin.com"
			,"instagram.com"
			,"pinterest.com"
			,"telegram.org"
			,"telegram.me"
			,"youtube.com"
			,"reddit.com"
			,"tumblr.com"
			,"flickr.com"
			,"plus.google.com"
		}

	{

    qDebug () << "WebCrawler constructed on thread:"
              << thread()
              << "Initializing variables...";

    m_urlQueue = urlQueue;

    // Initialize user-defined control variables and limits

	threadPool = new QThreadPool(this);
	threadPool->setExpiryTimeout(120000);

    m_discoveredNodes=1;                            // Counts discovered nodes -- Set the counter to 1, as we already know the initial url

    knownUrls[m_initialUrl]=m_discoveredNodes;      // Add the initial url to the map of known urls as node numbered 1.

    qDebug() << "initialUrl:" << m_initialUrl.toString()
             << " m_maxUrls " << m_maxUrls
             << " m_maxLinksPerPage " << m_maxLinksPerPage
             << " m_intLinks " << m_intLinks
             << " m_extLinksIncluded " << m_extLinksIncluded
             << " m_extLinksCrawl"<<m_extLinksCrawl
             << " m_socialLinks"<<m_socialLinks;

}


typedef bool (StringPredicate)(const QString&);
typedef std::function<StringPredicate> StrPredFun;

typedef QPair<int,int> Region;


struct reg_less {
	bool operator()(const Region& x, const Region& y) const {
		return x.second < y.first+1;
	}
};

void mergeRegions(QList<Region>& cutRegions) {
	std::set<Region, reg_less> regionSet;
	for (Region r: cutRegions) {
		Region cr = r;
		auto [begin, end] = regionSet.equal_range(cr);
		for (auto rit = begin; rit != end; ++rit) {
			const Region& mr = *rit;
			cr = qMakePair(
				std::min(cr.first, mr.first),
				std::max(cr.second, mr.second)
			);
		}
		regionSet.erase(begin, end);
		regionSet.insert(cr);
	}

	cutRegions.clear();
	for (const Region& e: regionSet) {
		cutRegions.append(e);
	}

}

void invertRegions(Region universe, const QList<Region>& regions, QList<Region>& result) {
	typedef std::set<Region, reg_less> myset;
	using myit = myset::iterator;
	myset regionSet(regions.begin(), regions.end());
	auto [begin, end] = regionSet.equal_range(universe);

	Region rest = universe;
	result.clear();

	for (myit it = begin; it != end; ++it) {
		const Region& r = *it;

		Region cross{std::max(r.first, rest.first), std::min(r.second, rest.second)};

		if (cross.first > rest.first) {
			result << Region(rest.first, cross.first);
		}
		rest = Region(cross.second, rest.second);
	}
	if (rest.second > rest.first) {
		result << rest;
	}
}

struct TagTriple {
	QString tag;
	QString id;
	int offs;
};

QString stripUnnessParts(QString& data) {
	QList<TagTriple> tagStack;
	QList<Region> cutRegions;

	StrPredFun translIdFilter = [](const QString& id) -> bool {
		return id.startsWith("Translations");
	};

	QList<StrPredFun> idFilters;
	idFilters << translIdFilter;

	static QRegularExpression tagRegex("<(?'TAG'/?\\w+)(?:\\s+id=(?'I'(?P>VAL))|\\s+[\\w-]+(?:=(?P>VAL))?)*\\s*>(?(DEFINE)(?'VAL'\"[^\"]*\"|'[^']*'|\\d+))");

	if (!tagRegex.isValid()) {
		qInfo() << "QRegularExpression invalid, error: " << tagRegex.errorString() << " at pos. " << tagRegex.patternErrorOffset();
	}

	QList<QRegularExpressionMatch> matches;
	QRegularExpressionMatchIterator i = tagRegex.globalMatch(data);
	while ( i.hasNext() ) {
		QRegularExpressionMatch match = i.next();
		QStringList parts = match.capturedTexts();

		QString tag = parts[1];
		QString tid = parts.length()>2 ? parts[2] : "";
		if (tid.size()>1) {
			if (tid.front()=='"')
				tid = tid.remove(0, 1);
			if (tid.back()=='"')
				tid.chop(1);
		}
		bool open = ! tag.startsWith('/');
		if (open) {
			int cutOffset = -1;
			if (!tid.isEmpty()) {
				for (const StrPredFun& pred: idFilters) {
					if (pred(tid)) {
						cutOffset = match.capturedStart();
						break;
					}
				}
			}
			tagStack.append({tag, tid, cutOffset});
		} else {
			tag = tag.remove(0, 1);
			TagTriple last;
			last.offs = -1;
			while (tag != last.tag && !tagStack.isEmpty()) {
				last = tagStack.back();
				tagStack.pop_back();
			}
			if (tag == last.tag && last.id.size() && last.offs >= 0) {
				int endOffs = match.capturedEnd();
				cutRegions << qMakePair(last.offs, endOffs);
			}
		}

	}

	mergeRegions(cutRegions);
	QList<Region> regionsToSave;
	invertRegions(Region(0, data.length()), cutRegions, regionsToSave);

	QString newData;
	{
		int newSize = 0;
		for (Region& r: regionsToSave) {
			newSize += r.second - r.first;
		}
		newData.reserve(newSize);
		for (Region& r: regionsToSave) {
			int sz = r.second - r.first;
			QStringRef ref(&data, r.first, sz);
			newData.append( ref );
		}
	}
	return newData;
}


/**
 * @brief Called from Graph when a network reply for a new page download has finished
 * to do the actual parsing of that page's html source from the reply bytearray.
 * First, we start by reading all from http reply to a QString called page.
 * Then we parse the page string, searching for url substrings.
 * @param reply
 */
void WebCrawler::parse(QNetworkReply *reply){
	QtConcurrent::run(threadPool, this, &WebCrawler::parseImpl, reply);
//	parseImpl(reply);
}

void WebCrawler::parseImpl(QNetworkReply *reply){

    qDebug () << "Parsing new network reply, on thread:" << this->thread();

    // Find the node the response HTML belongs to
    // Get this from the reply object request method
    QUrl currentUrl = reply->request().url();
    QString currentUrlStr = currentUrl.toString();
    QString locationHeader = reply->header(QNetworkRequest::LocationHeader).toString();
    QString scheme = currentUrl.scheme();
    QString host = currentUrl.host();
    QString path = currentUrl.path();

    int sourceNode = -1;
	{
		QReadLocker rl(&m_urlsLock);
	    sourceNode = knownUrls [ currentUrl ];
	}

    qDebug() << "Reply HTML belongs to url:" << currentUrlStr
             << " sourceNode:" << sourceNode
             << "host:" << host
             << "path:" << path;


    // Check for redirects
    if ( locationHeader != "" && locationHeader != currentUrlStr ) {
        qDebug () << "REDIRECT - Location response header:"
                  << locationHeader
                  << "differs from currentUrl:" << currentUrlStr
                  << "Calling newLink() to create node redirect, and RETURNing...";
        newLink( sourceNode, locationHeader , true );
        return;
    }

    QUrl newUrl;
    QString newUrlStr;
    int start=-1, end=-1, equal=-1 , invalidUrlsInPage =0; // index=-1;
    int validUrlsInPage = 0;

    QByteArray ba = reply->readAll();       // read all data from the reply into a bytearray
    reply->deleteLater();                   // schedule reply to be deleted
    QString page(ba);                       // construct a QString from the bytearray

    // Create a md5 hash of the page code
    QString md5(QCryptographicHash::hash(ba,QCryptographicHash::Md5).toHex());

    qDebug () << "Reply MD5 sum:" << md5.toLocal8Bit();

    // If there are no links inside the HTML source, return
    if (!page.contains ("href"))  {

        //FIXME: Frameset pages are not parsed! See docs/manual.html for example.

        qDebug() << "Empty or not useful html from:"
                 << currentUrl
                 << "page size:" << page.size()
                 << "\npage contents: " << page
                 << "\nRETURN ##";

        return;

    }

    //    qDebug() <<  " \npage contents: " << page << "\n\n";

    // We only search inside <body>...</body> tags
    qDebug() << "Finding <body></body> tags";

    start=page.indexOf ("<body");
    end=page.indexOf ("</body>");

    if ( start != -1 && end != -1 ) {
        page = page.remove(0, start);       // remove everything until <body>
        end=page.indexOf ("</body>");       // find new index pos of </body>
        page = page.left(end);              // keep everything until </body>
    }
    else if ( start == -1  ) {
        qDebug() << "ERROR IN opening <body> tag";
    }
    else if ( end == -1  ) {
        qDebug() << "ERROR IN locating closing </body> tag";
    }


	// remove whitespace from the start and the end
	// all whitespace sequence becomes single space
	page=page.simplified();

    if (currentUrl.hasFragment())  {
    	QString fragment = currentUrl.fragment();
    	QString frgProp = "id=\"" + fragment + '\"';
    	int frgPos = page.indexOf(frgProp);
    	if (frgPos<0)  {
	        qDebug() << "Missing fragment:"
                 << fragment
                 << " of a page :"
                 << currentUrl.toString()
                 << "\npage contents: " << page
                 << "\nRETURN ##";
	        return;
		}
		int frgStartTag = page.lastIndexOf('<', frgPos);
		int frgStartTagEnd = page.indexOf(' ', frgStartTag);
		QString tag = page.mid(frgStartTag, frgStartTagEnd - frgStartTag);
		static QRegularExpression nextTagReg( tag + R"~( [^<>]*id="([\w]+)")~" );
		if ( !nextTagReg.isValid() ) {
	        qDebug() << "Invalid next-tag-regexp: " << nextTagReg.errorString()
				<< ", pattern: /" << nextTagReg.pattern() << "/"
				<< ", position: " << nextTagReg.patternErrorOffset();
		}
		QString anchRef = " href=\"#";
		int nextFrgPos = -1;
		auto match = nextTagReg.match(page, frgPos + fragment.size());
		while (match.hasMatch()) {
			QString capture = match.captured(1);
			if ( page.contains(anchRef + capture) ) { // found next fragment
				nextFrgPos = match.capturedStart();
				break;
			}
		}
		page = page.mid(frgStartTag, nextFrgPos);
    }

    page = stripUnnessParts(page);

    // Main Loop: While there are more links in the page, parse them
    qDebug() << "Searching for href links inside the page code...";
    while (page.contains(" href") && !interrupted("parse")) {

        if (m_maxUrls>0) {
            if (m_discoveredNodes >= m_maxUrls ) {
                qDebug () <<"STOP because I have reached m_maxUrls.";
                emit finished("message from parse() -  discoveredNodes > maxNodes");
                return;
            }
        }

        start=page.indexOf (" href");		//Find its pos
        // Why " href" instead of "href"?
        // Because href might be inside random strings/tokens or
        // even in share links (ie. whatsapp://sadadasd &href=url)
        // By searching for " href" we avoid some of these cases, without
        // introducing other serious problems.

        page = page.remove(0, start);		//erase everything up to href

        equal=page.indexOf ("=");			// Find next equal sign (=)

        page=page.remove(0, equal+1);		//Erase everything up to =

        if (page.startsWith("\"") ) {
            page.remove(0,1);
            end=page.indexOf ("\"");
        }
        else if (page.startsWith("\'") ){
            page.remove(0,1);
            end=page.indexOf ("\'");
        }
        else {
            //end=page.indexOf ("\'");
        }

        newUrlStr=page.left(end);			//Save new url to newUrl :)

        newUrlStr=newUrlStr.simplified();
        qDebug() << "Found new URL:"<< newUrlStr << "Examining it...";

        newUrl = QUrl(newUrlStr);

        if (newUrl.isRelative()) {
            qDebug() << "newUrl is RELATIVE. Merging currentUrl with it.";
            newUrl = currentUrl.resolved(newUrl);
        }

        if (!newUrl.isValid()) {
            invalidUrlsInPage ++;
            qDebug() << "newUrl is INVALID: "
                        << newUrl.toString()
                        << "in page " << currentUrlStr
                        << "SKIPPING IT. Will continue in this page, only if invalidUrlsInPage < 200";
            if (invalidUrlsInPage > 200) {
                qDebug() << "RETURN because invalid newUrls > 200";
                emit finished("invalidUrlsInPage > 200");
                return;
            }
            continue;
        }

        // TODO - REMOVE LAST / FROM EVERY PATH NOT ONLY ROOT PATH
        if (newUrl.path() == "/") {
            newUrl.setPath("");
        }

        newUrlStr = newUrl.toString();

        qDebug() << "newUrl is VALID:" << newUrlStr
                 << "Checking if it is resource and if is allowed...";

        // Skip css, favicon, rss, ping, etc
        if ( newUrlStr.startsWith("#", Qt::CaseInsensitive) ||
             newUrlStr.endsWith("feed/", Qt::CaseInsensitive) ||
             newUrlStr.endsWith("rss/", Qt::CaseInsensitive) ||
             newUrlStr.endsWith("atom/", Qt::CaseInsensitive) ||
             newUrl.fileName().endsWith("xmlrpc.php", Qt::CaseInsensitive) ||
             newUrl.fileName().endsWith(".xml", Qt::CaseInsensitive) ||
             newUrl.fileName().endsWith(".ico", Qt::CaseInsensitive) ||
             newUrl.fileName().endsWith(".gif", Qt::CaseInsensitive) ||
             newUrl.fileName().endsWith(".png", Qt::CaseInsensitive) ||
             newUrl.fileName().endsWith(".jpg", Qt::CaseInsensitive) ||
             newUrl.fileName().endsWith(".js", Qt::CaseInsensitive) ||
             newUrl.fileName().endsWith(".css", Qt::CaseInsensitive) ||
             newUrl.fileName().endsWith(".rsd", Qt::CaseInsensitive)   )    {
            qDebug()<< "newUrl is a page resource or anchor (rss, favicon, etc) "
                        << "Skipping...";
            continue;
        }

        // Check if newUrl is compatible with the url patterns the user asked for
        bool urlPatternAllowed = true;

        for (const QString& elem: m_urlPatternsIncluded)  {

            QString urlPattern = elem.toLocal8Bit().constData();

            if (urlPattern.isEmpty()) {
                continue;
            }

            if ( newUrl.toString().contains( urlPattern ) ) {
                qDebug() << "newUrl in allowed url patterns:"
                          << urlPattern
                          << "OK.";
                break;
            }
            else {
                qDebug() << "newUrl not in allowed url patterns. I WILL SKIP IT. Pattern: " << urlPattern;
                urlPatternAllowed = false;
            }

        }


        bool urlPatternNotAllowed = false;

        if (urlPatternAllowed)
        for (const QString& pattern: m_urlPatternsExcluded) {
            QString urlPattern = pattern.toLocal8Bit().constData();

            if (urlPattern.isEmpty())
                continue;
            if ( newUrl.toString().contains( urlPattern ) ) {
                qDebug() << "newUrl in excluded url patterns:"
                          << urlPattern
                          << "I WILL SKIP IT.";
                urlPatternNotAllowed = true;
                break;
            }

        }
        if (urlPatternNotAllowed)
			qDebug() << "newUrl not in excluded url patterns. OK.";

        if (urlPatternAllowed && !urlPatternNotAllowed) {

            if ( newUrl.isRelative() ) {
                newUrl = currentUrl.resolved(newUrl);
                newUrlStr = newUrl.toString();

                qDebug() << "newUrl is RELATIVE."
                            << "host: " << host
                            << "resolved url:"
                            << newUrl.toString();

                if (!m_intLinks ){
                    qDebug()<< "SKIPPING node/edge creation because internal URLs are forbidden.";
                    continue;
                }

                if (currentUrl.path() == newUrl.path()) {
                    if  (m_selfLinks) {
                        qDebug()<< "Self-link found. Creating it!";

                        newLink(sourceNode, newUrl, false);
                    }
                    else {
                        qDebug()<< "Self-link found but self-links are not allowed. I will not create it.";
                    }

                }
                else {
                    qDebug()<< "Internal URLs allowed. Calling newLink() ";
                    this->newLink(sourceNode, newUrl, true);

                }
            }
            else {

                qDebug() << "newUrl is ABSOLUTE.";

                if ( newUrl.scheme() != "http"  && newUrl.scheme() != "https"  &&
                     newUrl.scheme() != "ftp" && newUrl.scheme() != "ftps") {
                    qDebug() << "INVALID newUrl SCHEME"
                                << newUrl.toString()
                                << "CONTINUE.";
                    continue;
                }

                if (  newUrl.host() != host  ) {
                    qDebug()<< "newUrl ABSOLUTE & EXTERNAL.";
                    if ( !m_extLinksIncluded ) {
                        qDebug()<< "External URLs forbidden. CONTINUE";
                        continue;
                    }
                    else {
                        bool urlIsSocial = false;
                        if ( !m_socialLinks ) {
							QString urlPattern;
                            for (const QString& link: m_socialLinksExcluded)  {
                                urlPattern = link.toLocal8Bit().constData();
                                if ( newUrl.host().contains ( urlPattern) ) {
                                    urlIsSocial = true;
                                    break;
                                }
                            }
                            if ( urlIsSocial) {
                                qDebug() << "newUrl in excluded social links:"
                                         << urlPattern
                                         << "CONTINUE ";
                                continue;
                            }
                        }
                        if ( m_extLinksCrawl ) {
                            qDebug()<< "External URLs are included and to be crawled. Calling newLink...";
                            newLink(sourceNode, newUrl, true);
                        }
                        else {
                            qDebug()<< "External URLs included but not to be crawled. Calling newLink() but the url will not be added to the queue";
                            newLink(sourceNode, newUrl, false);
                        }
                    }
                }
                else {
                    qDebug()<< "newUrl ABSOLUTE & INTERNAL.";

                    if (!m_intLinks){
                        qDebug()<< "Internal URLs forbidden."
                                  << "SKIPPING node creation";
                        continue;
                    }

                    if (  newUrl.path () == path && !m_selfLinks) {
                        qDebug()<< "Self links forbidden. CONTINUE.";
                        continue;
                    }

                    if ( newUrl.isParentOf(currentUrl) && !m_parentLinks ) {
                        qDebug()<< "Parent URLs forbidden. CONTINUE";
                        continue;
                    }
                    if ( currentUrl.isParentOf(newUrl) && !m_childLinks ) {
                        qDebug()<< "Child URLs forbidden. CONTINUE";
                        continue;
                    }

                    qDebug()<< "Internal, absolute newURL allowed. Calling newLink...";
                    newLink(sourceNode, newUrl, true);
                }
            }

			validUrlsInPage ++;
        } // end if in allowed patterns


        qDebug() << "validUrlsInPage:" << validUrlsInPage << "in page:" << currentUrlStr;

        // If the user has specified a maxLinksPerPage limit then,
        // if we have reached it, stop parsing this page
        if ( m_maxLinksPerPage  != 0 ) {
            if ( validUrlsInPage > m_maxLinksPerPage ) {
                qDebug () <<"STOP because I reached m_maxLinksPerPage "
                         <<m_maxLinksPerPage << " per page."  ;
                break;
            }
        }

    } // end while there are more links

}





/**
 * @brief ??
 * @param s
 * @param target
 * @param enqueue_to_frontier
 */
void WebCrawler::newLink(int s, QUrl target,  bool enqueue_to_frontier) {

    qDebug() << "Creating new edge, source:" <<  s << "target:" << target.toString();

    if (m_maxUrls>0 && m_discoveredNodes >= m_maxUrls ) {
            qDebug () <<"STOP because we have reached m_maxUrls!";
            emit finished("maxpages from newLink");
            return;
    }

    // check if the new url has been discovered previously
    bool urlExists = false;
    int linkInd = -1;
    {
		QReadLocker rl(&m_urlsLock);
		QMap<QUrl, int>::const_iterator index = knownUrls.constFind(target);
		urlExists = index != knownUrls.constEnd();
	    if ( urlExists ) {
			linkInd = index.value();
	    }
    }
    if ( urlExists ) {
        qDebug()<< "Target already discovered (in knownUrls) as node:" << linkInd;
		if (interrupted("newLink")) return;
        if  (s !=linkInd) {

            qDebug()<< "Emitting signalCreateEdge"
                    << s << "->"
                    << linkInd
                    << "and RETURN.";
            emit signalCreateEdge (s, linkInd );
        }
        else {
            qDebug()<< "Self links not allowed. RETURN.";
        }
        return;
    } else {
		QWriteLocker wl(&m_urlsLock);
		linkInd = m_discoveredNodes++;
		knownUrls[target] = linkInd;
    }

    if (interrupted("newLink")) return;

    qDebug()<< "Target just discovered. Emitting signalCreateNode:" << linkInd
            << "for url:"<< target.toString();

    emit signalCreateNode( linkInd, target.toString(), false);

    if (enqueue_to_frontier) {

        m_urlQueue->enqueue(target);

        qDebug()<< "Enqueued new node:" << linkInd <<  "to urlQueue."
                   << "queue size: "<<  m_urlQueue->size()
                   << " - Emitting signalStartSpider...";

        // Check if we need to add some delay between requests
        auto starter = [this]{
			if (interrupted("newLink")) return;
			emit signalStartSpider();
		};
        if (m_delayBetween) {
            int m_wait_msecs = rand() % m_delayBetween;
            qDebug() << "delay requested between signalStartSpider() calls. Setting a timer for:" << m_wait_msecs << "msecs";
            QTimer* timer = new QTimer();
            timer->singleShot(m_wait_msecs, this, starter);
            timer->callOnTimeout(timer, &QTimer::deleteLater);
        } else {
			starter();
        }
    }
    else {
        qDebug()<< "NOT adding new node to queue";
    }

	if (interrupted("newLink")) return;

    qDebug()<< "Emitting signalCreateEdge"
            << s << "->" << linkInd;

    emit signalCreateEdge (s, linkInd);

}

bool WebCrawler::interrupted(const char* func) {
    if ( QThread::currentThread()->isInterruptionRequested() ) {
        qDebug () <<"STOP because currentThread()->isInterruptionRequested() == true.";
        emit finished(QString("message from ") + func + "() -  interruptionRequested");
        return true;
    }
	return false;
}



WebCrawler::~WebCrawler() {

    qDebug() << "Destructor running. Clearing vars...";

    knownUrls.clear();
    m_discoveredNodes = 0;


}

