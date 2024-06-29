#pragma once

#include <windows.h>
#include <winuser.h>
#include <wingdi.h>
#include <vector>
#include <deque>
#include <unordered_set>

#define MIN_MARGINE 10

enum TConnectorDirection
{
	cdNothing,
	cdUp,
	cdUpRight,
	cdRight,
	cdDownRight,
	cdDown,
	cdDownLeft,
	cdLeft,
	cdUpLeft
};

struct TDelta
{
	int x;
	int y;
};
const TDelta aDir[] = { {0,0},{0,-1},{1,-1},{1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1} };
class CBaseObjects;
class CRect;
class Node;

struct TPixel
{
	bool bVisited;
	Node* pNode;
	CRect* pOccupied;
};

typedef std::vector<CBaseObjects*> TBaseObjectsArray;
typedef	std::vector<std::vector<TPixel>> TPixelArray;
typedef std::deque<Node*> TNodeDeque;

class CBaseObjects
{
public:
	CBaseObjects() :
		m_bIntersected(false)
	{}
	virtual void Draw(HDC dc) = 0;
	virtual void Clear(HDC dc) = 0;
	virtual bool IsIntersect(CBaseObjects* pObject) = 0;
	void SetIntersect(bool bIntersect) { m_bIntersected = bIntersect; }
	bool GetIntersected() { return m_bIntersected; }

public:
	void TestIntersected(HDC dc, TBaseObjectsArray aObjects, bool bIteratorDraw = false);

private:
	bool m_bIntersected;
};

class CRect: public CBaseObjects
{
public:
	CRect(RECT rCoordinates, COLORREF nBackground) :
		m_nBackground(nBackground),
		m_rCoordinates(rCoordinates),
		m_rCachedCoordinates(rCoordinates),
		m_bNeedUpdate(false),
		m_bMoving(false)
	{}
	~CRect() {}

public:
	virtual void Draw(HDC dc);
	virtual void Clear(HDC dc);
	virtual bool IsIntersect(CBaseObjects* pObject);

public:
	void MoveTo(int nDX, int nDY);
	POINT GetCenter();
	bool TestHit(int nX, int nY);
	const RECT GetRect() { return m_rCoordinates; }
	void SetMoving(bool bMoving) { m_bMoving = bMoving; }
	bool GetMoving() { return m_bMoving; }

private:
	COLORREF m_nBackground;
	RECT m_rCoordinates;
	RECT m_rCachedCoordinates;
	bool m_bNeedUpdate;
	bool m_bMoving;
};

class CConnector: public CBaseObjects
{
public:
	typedef std::vector<POINT> TPointsArray;

public:
	CConnector(COLORREF nPenColor, CRect* firstRect, CRect* secondRect, TPixelArray *aPixels):
		m_nPenColor(nPenColor),
		m_eDirection(cdNothing),
		m_aPixels(aPixels)
	{
		m_aRect.push_back(firstRect);
		m_aRect.push_back(secondRect);
	}
	~CConnector() {}

public:
	virtual void Draw(HDC dc);
	virtual void Clear(HDC dc);
	virtual bool IsIntersect(CBaseObjects* pObject);

public:
	void AddPoint(POINT point) { m_aPath.push_back(point); }
	void ClearPath();
	void BuildPath(HDC dc, TBaseObjectsArray *aRects);
	bool IsConnect(CRect* rect);

	//vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	struct AStarNode
	{
		POINT point;
		int gCost;
		int hCost;
		AStarNode* parent;

		AStarNode(POINT pt, int g, int h, AStarNode* par = nullptr) :
			point(pt), gCost(g), hCost(h), parent(par) {}

		int GetFCost() const { return gCost + hCost; }
	};

	//lllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll

private:
	void CalculateDirection();
	bool GetCloserIntersectedPoint(TBaseObjectsArray* aRects, RECT fromPoint, POINT& intersectPoint);


private:
	TPointsArray m_aPath;
	COLORREF m_nPenColor;
	std::vector<CRect*> m_aRect;
	std::vector<TPointsArray> m_aTryPathes;
	TConnectorDirection m_eDirection;
	TPixelArray *m_aPixels;
};

class CManager
{
public:
	CManager(HWND hWnd);
	~CManager();

public:
	int AddObject(RECT rCoordinates, COLORREF nColor);
	int AddConnector(COLORREF nColor, CRect* firstRect, CRect* secondRect);
	void RecalcPixels();
	CRect* GetObject(int nIndex);
	CConnector* GetConnector(int nIndex);
	CConnector* GetConnector();
	void BuildPathForAllConnectors();
	bool IsMoving() { return m_pMovingRect != nullptr; }
	bool TryToCaptureObject(int nStartX, int nStartY);
	void BeginMove(int nStartX, int nStartY);
	void EndMove();
	void MoveTo(int nX, int nY);
	void ReDrawAll();
	void Draw(bool bRectOnly = false);
	void ClearAll();
	void ShowOccupied();

private:
	void SetAreaOccupied(CBaseObjects *pObect, bool bOccupied);

private:
	CRect* m_pMovingRect;
	TBaseObjectsArray m_aObjects;
	TBaseObjectsArray m_aConnectors;
	int m_nStartX;
	int m_nStartY;
	HWND m_nhWnd;
	TPixelArray m_aPixels;
};

