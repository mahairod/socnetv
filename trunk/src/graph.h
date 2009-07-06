/***************************************************************************
 SocNetV: Social Networks Visualizer 
 version: 0.80
 Written in Qt 4.4
 
                         graph.h  -  description
                             -------------------
    copyright            : (C) 2005-2009 by Dimitris B. Kalamaras
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

#ifndef GRAPH_H
#define GRAPH_H


#include <QObject>
#include <QList>
#include <QTextStream>


#include <stack>  //FYI: stack is a wrapper around <deque> in C++, see: www.cplusplus.com/reference/stl/stack
#include <map>



#include "vertex.h"
#include "matrix.h"
#include "parser.h"
#include "webcrawler.h"


using namespace std;


class QPointF;
class QDateTime;

/**	This is the main class for a Graph, used in conjuction with Vertex, Parser and Matrix objects.
	
	Graph class has the interface and the various network algorithms 
	Vertex class holds each vertex data (colors, strings, statistics, etc)
	Matrix class holds the adjacency matrix of the network.
	Parser class loads files of networks.
	
*/

typedef map<float,int> fmap_i;
typedef map<int,int> imap_i;
typedef map<int,float> imap_f;
typedef QList<Vertex*> Vertices;

class Graph:  public QObject{
	Q_OBJECT

public slots:
	/** Slots to signals from Parser */
	void createVertex(int,int,QString, QString, QString, QPointF, QString); 	//Main vertex creation call
	void setFileType(int, QString, int,int, bool);	
	void removeDummyNode(int);

	/** Slots to signals from GraphicsWidget and Parser*/
	void createEdge (int, int, float, QString, bool, bool, bool);		//GW and Parser.
	void createEdge (int, int, float, bool, bool, bool);				//GW
	void createEdge (int, int);											//WebCrawler
	void nodeMovement(int state, int type, int cW, int cH);			//Called by MW to start movement

	void slotSetEdgeVisibility( int, int, bool);
	
	void createVertex(int i, QPointF p); 							//Called by GW
	void createVertex(int i, int canvasWidth, int canvasHeight); 	//Called by MW
	void createVertex(QString label, int i) ; 						//Called by WebCrawler
	
	/** Slots to signals from MainWindow */
	void setCanvasDimensions(int w, int h);				
	void filterEdgesByWeight (float, bool);				//Called by MW to filter edges over/under a weight
	
	
	void webCrawl( QString, int, int, bool);	//Called by MW to start a web crawler...

	
signals:
	/** Signals to MainWindow */
	void updateProgressDialog(int );
	void graphChanged();								//call to update MW widgets
	void selectedVertex(int);							//notifies MW who is the selected node
	void signalFileType (int, QString, int,int, bool); //notifies MW what we have loaded.
	void statusMessage (QString message);				//updates statusbar message
	
	/** Signals to GraphicsWidget */
	void drawNode( int ,int,  QString, QString,QString, QPointF, QString, bool, bool, bool);	//call GW to draw a node
	void eraseNode (int);		//erase node from GW  
	void drawEdge(int, int, float, bool, bool, QString, bool, bool);	//call GW to draw an edge
	void eraseEdge(int, int);		//emited from removeEdge() to GW to clear the edge item.
	void setEdgeVisibility ( int, int, bool); // emitted from each Vertex 
	
	void drawEdgeReciprocal(int, int);		//call GW to draw the edge as symmetric one
	void addBackgrCircle(int, int, int);	//call GW to draw a circular layout line somewhere.
	void addBackgrHLine (int);				//call GW to draw a horizontal layout line somewhere.
	void moveNode(int, int, int);

	
public: 	
	/**INIT AND CLEAR*/
	Graph(); 				//Creates a new graph.
	void clear();			//Clears m_graph 
	~Graph();				//destroy
	
	void setShowLabels(bool toggle);
	void setShowNumbersInsideNodes(bool toggle);

	/**FILES (READ AND WRITE)*/
	bool loadGraph (	
					QString, int, QString, 
					QString, QString, bool, 
					int maxWidth, int maxHeight
				);	//Our almost universal network loader. :)
	
	bool saveGraph( QString fileName, int fileType, 
						QString networkName, int maxWidth, int maxHeight 
				);
	bool saveGraphToPajekFormat (QString fileName,QString networkName,  int maxWidth, int maxHeight);
	bool saveGraphToAdjacencyFormat (QString fileName, int maxWidth, int maxHeight);
	bool saveGraphToDotFormat (QString fileName, QString networkName, int maxWidth, int maxHeight);
	bool saveGraphToGraphMLFormat (QString fileName,QString networkName,  int maxWidth, int maxHeight);

	/** VERTICES */
	int lastVertexNumber();						//Returns the number of the last vertex
	int firstVertexNumber();					//Returns the number of the first vertex

	int hasVertex(int );						//Checks if a vertex exists
	int hasVertex(QString);						//Checks if a vertex with a label exists
	void removeVertex (int );					//removes given vertex from m_graph

	void setInitVertexSize (int); 				//Changes the init size used by all new vertices.
	void setVertexSize(int v, int );			//Changes the size.of vertex v 

	void setInitVertexShape (QString); 			//Changes the init shape used by all new vertices.
	void setVertexShape(int v, QString shape); 	//Changes the shape.of vertex v 
	QString shape(int v);						//returns the shape of this vertex

	void setInitVertexColor (QString color);  	//Changes the init color used by all new vertices
	void setVertexColor(int v, QString color); 	//Changes the color.of vertex v 

	void setInitVertexLabelColor(QString color);//Changes the init color used by all new vertices' labels
	void setVertexLabel(int v, QString label); 	//Changes the label.of vertex v 
	QString label(int);			

	void updateVertCoords(int v, int x, int y);	 //Updates vertex v with coords x,y

	int vertices() ;							//Returns the sum of vertices inside m_graph

	int edgesFrom (int i) ;						//Returns the number of edges starting from v1 (outDegree)
	int edgesTo (int i) ;						//Returns the number of edges ending to v1 (inDegree)  

	int verticesWithOutEdges();					//Returns the sum of vertices having outEdges
	int verticesWithInEdges();					//Returns the sum of vertices having inEdges
	int verticesWithReciprocalEdges();			//Returns the sum of vertices having reciprocal edges


	/**EDGES*/
	float hasEdge (int v1, int v2);				//Checks if edge between v1 and v2 exists. Returns weight or -1
	void removeEdge (int v1, int v2);			//removes the edge between v1 and v2
	
 
	void setEdgeWeight (int v1, int v2, float w); 	//Sets the edge weight between v1 and v2
	void setInitEdgeColor(QString);

	void setEdgeColor(int s, int t, QString color);	//Changes the color of edge (s,t).
	QString edgeColor (int s, int t); 				//Returns the edgeColor
	 
	int totalEdges ();							//Returns the sum of edges inside m_graph

	float density();							//Returns ratio of present edges to total possible edges.

	bool symmetricEdge(int v1, int v2);			//Returns TRUE if (v1, v2) is symmetric.
	bool isSymmetric();							//Returns TRUE if symmetricAdjacencyMatrix=TRUE
	void symmetrize();							//Symmetrize all edges so that the network is undirected.


	/**PRINT OUT*/
	void writeAdjacencyMatrixTo(QTextStream& os);	 			//Exports the adjacency matrix to a given textstream
	void writeAdjacencyMatrix(const char*, const char*);		//Writes the adjacency matrix to a given file.
	void writeDistanceMatrix(const char*, const char*, const char*);//Writes the distance matrix to a file
	friend QTextStream& operator <<  (QTextStream& os, Graph& m);  	//

	void writeCentralityInDegree(const QString, bool);				//Writes the in-degree centralities to a file
	void writeCentralityOutDegree(const QString, const bool);		//Writes the out-degree centralities to a file
	void writeCentralityCloseness(const QString, const bool);		//Writes the closeness centralities to a file
	void writeCentralityBetweeness(const QString, const bool);		//Writes the betweeness centralities to a file
	void writeCentralityGraph(const QString, const bool);		//Writes the Graph centralities to a file
	void writeCentralityStress(const QString, const bool);		//Writes the Stress centralities to a file	
	void writeCentralityEccentricity(const QString, const bool);		//Writes the Eccentr centralities to a file
	void writeClusteringCoefficient(const QString, const bool);		//Writes the clustering coefficients to a file
	
	


	/**DISTANCES & CENTRALITIES*/
	int distance( int, int);					//Returns the geodesic distance between two vertices
	int diameter();								//Returns the diameter of the graph (maximum shortest path).
	float averageGraphDistance();					//Returns the average shortest path length (average geodesic).

	void createDistanceMatrix(bool);			//Creates the distance matrix and calculates the centralities, if bool is true.

	void centralityInDegree(bool);				//Calculates the inDegree centrality of each vertex
	void centralityOutDegree(bool);				//Calculates the outDegree centrality of each vertex


	float numberOfTriples(int v1); 		//Returns the number of triples at vertex v1
	float numberOfCliques(int v1);		//Calculates the number of cliques (triangles) of vertex v1
	float numberOfCliques();			//Calculates the number of cliques (triangles) of the whole graph
	float clusteringCoefficient(int v1);	
	float clusteringCoefficient ();
	
//	void eccentr_JordanCenter(); 				// TODO


	/**LAYOUTS*/	
	void layoutRandom(double maxWidth, double maxHeight);
	void layoutCircleCentrality(double x0, double y0, double maxRadius, int CentralityType);
	void layoutLevelCentrality(double maxWidth, double maxHeight, int CentralityType);
	void layoutForceDirectedSpringEmbedder(bool dynamicMovement);
	void layoutForceDirectedFruchtermanReingold(bool dynamicMovement);

	/**RANDOM NETWORKS*/
	void createRandomNetErdos 
			(int, double);				//Creates a uniform random network
	
	void createRandomNetRingLattice
			(int, int, double, double, double); 	//Creates a ring lattice network
				
	void createSameDegreeRandomNetwork
			(int, int); 				//Creates a random network with the same degree in all nodes
				
	void createRandomNetSmallWorld 
			(int, int, double, double, double, double); 	//Creates a small world network
				
	int factorial (int);				// for  (n 2)p edges calculation


	/*
	*   index stores the index of each vertex inside m_graph. It starts at zero (0).
	*   This is crucial when we want to find the place of a vertex inside m_graph after adding or removing many vertices 
	*/
	imap_i index;			
	/** maps have O(logN) lookup complexity
		Consider using tr1::hashmap which has O(1) lookup, but this is not ISO C++ yet :(  
	*/





protected: 

	void timerEvent(QTimerEvent *event);			// Called from nodeMovement when a timerEvent occurs
private:

	/** List of pointers to the vertices. A vertex stores all the info: links, colours, etc */
	Vertices m_graph;			

	Parser parser;			//file loader threaded class.

	WebCrawler crawler; 
	
	/** private member functions */
	void addVertex (int v1, int val, int nsz, QString nc, QString nl, QString lc, QPointF p,QString nsp);	
	void addEdge (int v1, int v2, float w, QString color, bool undirected); 		//adds an edge between v1 and v2, weight w, colored

	/** methods used by createDistanceMatrix()  */
	void BFS(int, bool);									//Breadth-first search 
	void minmax(float C, Vertex *v, float &max, float &min, int &maxNode, int &minNode) ;	//helper
	void resolveClasses(float C, fmap_i &discreteClasses, int &classes);			//helper
	void resolveClasses(float C, fmap_i &discreteClasses, int &classes, int name);  	//helper

	/** used in createDistanceMatrix() */
	fmap_i	discreteIDCs, discreteODCs, discreteCCs, discreteBCs, discreteSCs, discreteGCs, discreteECs;
	int *eccentricities;
	bool calculatedIDC, calculatedODC, calculatedCentralities, dynamicMovement;
	Matrix  TM, DM;
	stack<int> Stack;

	float meanDegree, varianceDegree;
	float minIDC, maxIDC, sumIDC, groupIDC;
	float minODC, maxODC, sumODC, groupODC;
	float minCC, maxCC, nomCC, denomCC, sumCC, groupCC, maxIndexCC;
	float minBC, maxBC, nomBC, denomBC, sumBC, groupBC, maxIndexBC;
	float minGC, maxGC, nomGC, denomGC, sumGC, groupGC, maxIndexGC;
	float minSC, maxSC, nomSC, denomSC, sumSC, groupSC, maxIndexSC;
	float minEC, maxEC, nomEC, denomEC, sumEC, groupEC, maxIndexEC;
	float minCLC, maxCLC, averageCLC;
	int maxNodeCLC, minNodeCLC;
	int classesIDC, maxNodeIDC, minNodeIDC;
	int classesODC, maxNodeODC, minNodeODC;
	int classesCC, maxNodeCC, minNodeCC;
	int classesBC, maxNodeBC, minNodeBC;
	int classesGC, maxNodeGC, minNodeGC;
	int classesSC, maxNodeSC, minNodeSC;
	int classesEC, maxNodeEC, minNodeEC;


	/** General & initialisation variables */

	int m_totalEdges, m_totalVertices, graphDiameter, initVertexSize;
	float averGraphDistance, nonZeroDistancesCounter;
	int outEdgesVert, inEdgesVert, reciprocalEdgesVert;
	int timerId,  layoutType, canvasWidth, canvasHeight;
	
	bool order, initShowLabels, initNumbersInsideNodes;
	bool adjacencyMatrixCreated, symmetricAdjacencyMatrix, graphModified, distanceMatrixCreated;
	bool m_undirected;

	QString networkName, initEdgeColor, initVertexColor, initVertexLabelColor, initVertexShape;
	
	QDateTime actualDateTime;
};

#endif
