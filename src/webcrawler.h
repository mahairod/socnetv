/***************************************************************************
 SocNetV: Social Networks Visualizer
 version: 1.6
 Written in Qt
 
                         webcrawler.h  -  description
                             -------------------
    copyright            : (C) 2005-2015 by Dimitris B. Kalamaras
    email                : dimitris.kalamaras@gmail.com
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

#ifndef WEBCRAWLER_H
#define WEBCRAWLER_H

using namespace std;

#include <QThread>
#include <QNetworkReply>


// CONSUMER
class WebCrawler_Parser : public QObject  {
	Q_OBJECT
public:  
	void createNode(QString url, bool enqueue_to_frontier);

public slots:
    void parse(QNetworkReply *reply);
signals:
	void signalCreateNode(QString url, int no);
	void signalCreateEdge (int source, int target);
};


// PRODUCER CLASS
class WebCrawler_Spider : public QObject  {
    Q_OBJECT
public slots:
    void get();
signals:
    void createNode(QString url, int no);
};


class WebCrawler :  public QObject {
	Q_OBJECT
    QThread parserThread;
    QThread spiderThread;
public:
	void load(QString seed, int maxNodes, int maxRecursion, bool goOut);
	void terminateReaderQuit ();
public slots:
	void slotCreateNode(QString url, int no);
	void slotCreateEdge (int source, int target);
signals:
	void createNode(QString url, int no);
	void createEdge (int source, int target);
    void operateParser();
    void operateSpider();
private: 
	QString url;
    WebCrawler_Parser *wc_parser;
    WebCrawler_Spider *wc_spider;
};
#endif
