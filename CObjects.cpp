#include "CObjects.h"
#include "Graph.h"
//----------------
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <optional>
//#include <stack>
#include <map>
#include <cstddef> // ƒл€ std::size_t

#include <windows.h>
#include <wincodec.h>
#include <gdiplus.h>
//-----------------


static bool IsIntersect(RECT o, RECT r)
{
	if (
		(o.left >= r.left && o.top >= r.top && o.left <= r.right && o.top <= r.bottom) || //—тартова€ точка отрезка внутри
		(o.right >= r.left && o.bottom >= r.top && o.right <= r.right && o.bottom <= r.bottom) //‘инишна€ точка отрезка внутри
		)
		return true;//ќдин из концов отрезка внутри

	if (o.top == o.bottom) //√оризонтальный отрезок
		if ((o.left < r.left && o.right > r.right) && //X по разные стороны
			(o.top >= r.top && o.bottom <= r.bottom)) //Y внутри
			return true;

	if (o.left == o.right) //¬ертикальный отрезок
		if ((o.top < r.top && o.bottom > r.bottom) && //Y по разные стороны
			(o.left >= r.left && o.right <= r.right)) //X внутри
			return true;

	return false;
}

CManager::CManager(HWND hWnd) :
	m_pMovingRect(nullptr),
	m_nhWnd(hWnd),
	m_aObjects(0),
	m_aConnectors(0),
	m_nStartX(0),
	m_nStartY(0)
{
	RECT rect;
	GetClientRect(m_nhWnd, &rect);
	m_aPixels.resize(rect.right+1);
	for (auto i = 0; i <= rect.right; i++)
	{
		m_aPixels[i].resize(rect.bottom+1);
		for (auto j = 0; j <= rect.bottom; j++)
			m_aPixels[i][j] = { false,nullptr,nullptr };
	}
	return;
}

CManager::~CManager()
{
	ClearAll();
}

void CManager::ClearAll()
{
	HDC dc = GetDC(m_nhWnd);

	for (auto itr : m_aObjects)
	{
		itr->Clear(dc);
		delete itr;
	}
	m_aObjects.clear();

	for (auto itr : m_aConnectors)
	{
		itr->Clear(dc);
		delete itr;
	}
	m_aConnectors.clear();

	ReleaseDC(m_nhWnd, dc);

}

CRect* CManager::GetObject(int nIndex)
{
	if (nIndex < m_aObjects.size())
		return (CRect*)m_aObjects[nIndex];
	return nullptr;
}

CConnector* CManager::GetConnector()
{
	if (m_pMovingRect)
	{
		for (auto itr : m_aConnectors)
			if (((CConnector*)itr)->IsConnect(m_pMovingRect))
				return (CConnector*)itr;
	}
	return nullptr;
}

CConnector* CManager::GetConnector(int nIndex)
{
	if (nIndex < m_aConnectors.size())
		return (CConnector*)m_aConnectors[nIndex];
	return nullptr;
}

int CManager::AddObject(RECT rCoordinates, COLORREF nColor)
{
	m_aObjects.push_back(new CRect(rCoordinates, nColor));
	return m_aObjects.size() - 1;
}

int CManager::AddConnector(COLORREF nColor, CRect* firstRect, CRect* secondRect)
{
	m_aConnectors.push_back(new CConnector(nColor, firstRect, secondRect, &m_aPixels));
	return m_aConnectors.size() - 1;
}

bool CManager::TryToCaptureObject(int nStartX, int nStartY)
{
	for (auto itr : m_aObjects)
		if (((CRect*)itr)->TestHit(nStartX, nStartY))
		{
			m_pMovingRect = (CRect*)itr;
			m_pMovingRect->SetMoving(true);
			BeginMove(nStartX, nStartY);
			return true;
		}
	return false;
}

void CManager::Draw(bool bRectOnly/* = false*/)
{
	HDC dc = GetDC(m_nhWnd);

	if (dc)
	{
	for (auto rect : m_aObjects)
	{
		if (m_pMovingRect != rect)
		{
			SetAreaOccupied(rect, true);
			rect->TestIntersected(dc, m_aObjects);
			rect->Draw(dc);
			if (rect->IsIntersect(m_pMovingRect))
				m_pMovingRect->Draw(dc);
		}
	}

	if (!bRectOnly)
		for (auto conn : m_aConnectors)
		{
			conn->TestIntersected(dc, m_aObjects, true);
			conn->Draw(dc);
		}

	ReleaseDC(m_nhWnd, dc);

	}
	
}

void CManager::ReDrawAll()
{
	RECT winRect;
	GetClientRect(m_nhWnd, &winRect);
	HDC dc = GetDC(m_nhWnd);
	HBRUSH brush = CreateSolidBrush(GetDCBrushColor(dc));
	FillRect(dc, &winRect, brush);
	DeleteObject(brush);
	Draw();
	ReleaseDC(m_nhWnd, dc);
}

void CManager::MoveTo(int nX, int nY)
{
	if (m_pMovingRect)
	{
		m_pMovingRect->SetIntersect(false);
		SetAreaOccupied(m_pMovingRect, false);
		m_pMovingRect->MoveTo(nX - m_nStartX, nY - m_nStartY);
		m_nStartX = nX;
		m_nStartY = nY;

		HDC dc = GetDC(m_nhWnd);
		if (dc)
		{
			CConnector* pConnector = GetConnector();
			if (nullptr != pConnector)
			{
				pConnector->Clear(dc);
				pConnector->BuildPath(dc, &m_aObjects);
				pConnector->TestIntersected(dc, m_aObjects, true);
				pConnector->Draw(dc);
			}

			Draw();

			m_pMovingRect->Draw(dc);
			SetAreaOccupied(m_pMovingRect, true);

			ReleaseDC(m_nhWnd, dc);
		}
	}
}

void CManager::BeginMove(int nStartX, int nStartY)
{
	m_nStartX = nStartX;
	m_nStartY = nStartY;
}

void CManager::EndMove()
{
	m_nStartX = m_nStartY = 0;
	if (m_pMovingRect)
		m_pMovingRect->SetMoving(false);
	m_pMovingRect = nullptr;
}

void CManager::BuildPathForAllConnectors()
{
	HDC dc = GetDC(m_nhWnd);

	for (auto itr : m_aConnectors)
		((CConnector*)itr)->BuildPath(dc, &m_aObjects);
	ReleaseDC(m_nhWnd,dc);
}


void CManager::SetAreaOccupied(CBaseObjects* pObect, bool bOccupied)
{
	RECT rect = ((CRect*)pObect)->GetRect();
	if (rect.left < MIN_MARGINE)
		rect.left = 0;
	else
		rect.left -= MIN_MARGINE;
	if (rect.right + MIN_MARGINE > m_aPixels.size()-1)
		rect.right = m_aPixels.size() - 1;
	else
		rect.right += MIN_MARGINE;
	if (rect.top < MIN_MARGINE)
		rect.top = 0;
	else
		rect.top -= MIN_MARGINE;
	if (rect.bottom + MIN_MARGINE > m_aPixels[0].size() - 1)
		rect.bottom = m_aPixels[0].size() - 1;
	else
		rect.bottom += MIN_MARGINE;

	for (auto i = rect.left; i <= rect.right; i++)
		for (auto j = rect.top; j <= rect.bottom; j++)
			if (bOccupied)
				m_aPixels[i][j].pOccupied = (CRect*)pObect;
			else
				m_aPixels[i][j].pOccupied = nullptr;

}

void CManager::RecalcPixels()
{
	for (auto itr : m_aObjects)
		SetAreaOccupied(itr, true);
}

void CManager::ShowOccupied()
{
	
	HDC dc = GetDC(m_nhWnd);
	if (dc)
	{
		RECT rect = { 0,0,m_aPixels.size() - 1,m_aPixels[0].size() - 1 };

		HBRUSH brush = CreateSolidBrush(0x00FF00);

		FillRect(dc, &rect, brush);
		DeleteObject(brush);

		brush = CreateSolidBrush(0x0000FF);
		HBRUSH brush1 = CreateSolidBrush(0xFF00FF);
		for (auto i = rect.left; i <= rect.right; i++)
			for (auto j = rect.top; j <= rect.bottom; j++)
			{
				if (m_aPixels[i][j].pOccupied)
				{
					RECT r = { i,j,i + 1,j + 1 };
					FillRect(dc, &r, brush);
				}
				if (m_aPixels[i][j].pNode)
				{
					RECT r = { i,j,i + 1,j + 1 };
					FillRect(dc, &r, brush1);
				}
			}
		DeleteObject(brush);
		DeleteObject(brush1);
		ReleaseDC(m_nhWnd,dc);
	}

}


void CBaseObjects::TestIntersected(HDC dc, TBaseObjectsArray aObjects, bool bIteratorDraw/* = false*/)
{
	SetIntersect(false); 
	for (auto test : aObjects)
		if (test != this)
			if (IsIntersect(test))
			{
				SetIntersect(true);
				if (bIteratorDraw)
					test->Draw(dc);
				else
					break;
			}
}

//------------------------------------------------------------------------------------------------------------------
void CRect::Draw(HDC dc)
{
	if (m_bNeedUpdate)
		Clear(dc);

	HBRUSH brush = nullptr;

	if (GetIntersected())
		brush = CreateHatchBrush(5, m_nBackground);
	else
		brush = CreateSolidBrush(m_nBackground);


	FillRect(dc, &m_rCoordinates, brush);

	
	DeleteObject(brush);

	m_rCachedCoordinates = m_rCoordinates;
	m_bNeedUpdate = false;
}

void CRect::Clear(HDC dc)
{
	HBRUSH brush = CreateSolidBrush(GetDCBrushColor(dc));
	FillRect(dc, &m_rCachedCoordinates, brush);
	DeleteObject(brush);
}

void CRect::MoveTo(int nDX, int nDY)
{
	if (!m_bNeedUpdate)
		m_rCachedCoordinates = m_rCoordinates;

	m_rCoordinates.left += nDX;
	m_rCoordinates.right += nDX;
	m_rCoordinates.top += nDY;
	m_rCoordinates.bottom += nDY;

	m_bNeedUpdate = true;
}


POINT CRect::GetCenter()
{
	return { (m_rCoordinates.right - m_rCoordinates.left) / 2 + m_rCoordinates.left,
			 (m_rCoordinates.bottom - m_rCoordinates.top) / 2 + m_rCoordinates.top };
}

bool CRect::TestHit(int nX, int nY)
{
	return	nX >= m_rCoordinates.left && nX <= m_rCoordinates.right &&
			nY >= m_rCoordinates.top && nY <= m_rCoordinates.bottom;
}

bool CRect::IsIntersect(CBaseObjects* pObject)
{
	if (pObject)
	{
		CRect* rect = (CRect*)pObject;
		if (rect == this)
			return false;
		RECT r = rect->GetRect();
		if (::IsIntersect({ m_rCoordinates.left, m_rCoordinates.top, m_rCoordinates.right, m_rCoordinates.top }, r))
			return true;
		if (::IsIntersect({ m_rCoordinates.right, m_rCoordinates.top, m_rCoordinates.right, m_rCoordinates.bottom }, r))
			return true;
		if (::IsIntersect({ m_rCoordinates.left, m_rCoordinates.bottom, m_rCoordinates.right, m_rCoordinates.bottom }, r))
			return true;
		if (::IsIntersect({ m_rCoordinates.left, m_rCoordinates.top, m_rCoordinates.left, m_rCoordinates.bottom }, r))
			return true;
	}
	return false;
}

//--------------------------------------------------------------------------------
void CConnector::CalculateDirection()
{
	if (m_aRect.size() != 2)
		return;
	POINT s = m_aRect[0]->GetCenter();
	POINT f = m_aRect[1]->GetCenter();
	if (s.x == f.x && s.y == f.y)
		m_eDirection = cdNothing;
	else if (s.x == f.x && s.y > f.y)
		m_eDirection = cdUp;
	else if (s.x < f.x && s.y > f.y)
		m_eDirection = cdUpRight;
	else if (s.x < f.x && s.y == f.y)
		m_eDirection == cdRight;
	else if (s.x < f.x && s.y < f.y)
		m_eDirection = cdDownRight;
	else if (s.x == f.x && s.y < f.y)
		m_eDirection = cdDown;
	else if (s.x > f.x && s.y < f.y)
		m_eDirection = cdDownLeft;
	else if (s.x > f.x && s.y == f.y)
		m_eDirection = cdLeft;
	else if (s.x > f.x && s.y > f.y)
		m_eDirection = cdUpLeft;
	else
		m_eDirection = cdNothing;
}

bool CConnector::GetCloserIntersectedPoint(TBaseObjectsArray* aRects, RECT fromPoint, POINT& intersectPoint)
{
	return false;
}

//vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

//AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA

void CConnector::ClearPath()
{
	m_aPath.clear();
	m_aTryPathes.clear(); 
	for (auto i = 0; i < (*m_aPixels).size(); i++)
		for (auto j = 0; j < (*m_aPixels)[i].size(); j++)
			(*m_aPixels)[i][j].pNode = nullptr;
}

void CConnector::BuildPath(HDC dc, TBaseObjectsArray* aRects)
{

	ClearPath();

	TNodeDeque node1PriorityDeque;
	TNodeDeque node2PriorityDeque;
	TNodeDeque node3PriorityDeque;
	Node* pFinishNode = nullptr;
	Node* pStartNode = new Node(m_aRect[0]->GetCenter(), m_aRect[1]->GetCenter(), 0, &m_aRect, m_aPixels);
	pFinishNode = pStartNode->BuldEdges(node1PriorityDeque, node2PriorityDeque, node3PriorityDeque);
	bool bBreak = false;
	while (!pFinishNode && !(node1PriorityDeque.empty() && node2PriorityDeque.empty() && node3PriorityDeque.empty()))
	{
		while (!node1PriorityDeque.empty())
		{
			Node* pNode = node1PriorityDeque.front();
			pFinishNode = pNode->BuldEdges(node1PriorityDeque, node2PriorityDeque, node3PriorityDeque);
			node1PriorityDeque.pop_front();
		}
		if (!pFinishNode)
			if (!node2PriorityDeque.empty())
			{
				Node* pNode = node2PriorityDeque.front();
				pFinishNode = pNode->BuldEdges(node1PriorityDeque, node2PriorityDeque, node3PriorityDeque);
				node2PriorityDeque.pop_front();
			}
		if (!pFinishNode)
			if (node1PriorityDeque.empty() && node2PriorityDeque.empty())
				if (!node3PriorityDeque.empty())
				{
					Node* pNode = node3PriorityDeque.front();
					pFinishNode = pNode->BuldEdges(node1PriorityDeque, node2PriorityDeque, node3PriorityDeque);
					node3PriorityDeque.pop_front();
				}
	}

	if (pFinishNode)
		while (pFinishNode->prevIndex)
		{
			m_aPath.insert(m_aPath.begin(), pFinishNode->m_Start);
			pFinishNode = pFinishNode->prevIndex;
		}
	/*
	ClearPath();
	CalculateDirection();

	POINT fromPoint = m_aRect[0]->GetCenter();
	POINT toPoint = m_aRect[1]->GetCenter();

	AddPoint(fromPoint);

	//vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	for (auto i = rect.left; i <= rect.right; i++)
		for (auto j = rect.top; j <= rect.bottom; j++)
			if (!m_aPixels[i][j].bOccupied)
			{
				
			}

	std::vector<std::size_t> find_path_Dijkstra(Graph & graph, std::size_t start_index, std::size_t end_index)
	{
		graph.init_start_values(); // прометим в графе стартовые значени€
		//std::map<int, int> min_weigth_map;  // - замен€ем на multimap ->
		std::multimap<int, std::size_t> min_weigth_map; // структура данных вместо очереди с приоритетами (map - бинарное дерево поиска, данные в нем отсортированы )
		graph.m_aPixels[start_index].weight = 0; // стартова€ точка отсчЄта
		min_weigth_ma[0] = start_index;  // - вместо этого пишем ->
		min_weigth_map.insert({ 0, start_index });
		while (!min_weigth_map.empty()) // пока мапа не пуста извлекаем из неЄ последнюю вершину
		{
			auto [current_weight, current_index] = *(min_weigth_map.begin());
			//auto current = *(min_weigth_map.begin());
			//int current_weight = current.first;
			//int current_index = current.second
			min_weigth_map.erase(min_weigth_map.begin()); // удал€ем из мапы это значение - оно более не нужно
			if (graph.m_aPixels[current_index].visited) // если вершина посещЄнна€, то идЄм дальше
			{
				continue;
			}

			graph.m_aPixels[current_index].visited = true; // обработанную вершину помечаем как посещЄнную

			for (std::size_t i = 0; i < graph.m_aPixels[current_index].edges.size(); i++)  // идем по вершинам, соседним текущей
			{
				std::size_t index_to = graph.m_aPixels[current_index].edges[i].indexTo;
				int edged_weight = graph.m_aPixels[current_index].edges[i].weight;
				if (!graph.m_aPixels[index_to].visited) // если натыкаемс€ на вершину с меньшим весом, чем текущий, то записываем в текущие новый вес, индекс и закидываем вершину в мапу
				{
					if (current_weight + edged_weight < graph.m_aPixels[index_to].weight)
					{
						graph.m_aPixels[index_to].weight = current_weight + edged_weight;
						graph.m_aPixels[index_to].prevIndex = current_index;
						min_weigth_map.insert({ graph.m_aPixels[index_to].weight, index_to }); // заносим текущую вершину и ее рассто€ние
					}
				}
			}

		}

		return convert_graph_to_path(graph, start_index, end_index); // конвертаци€ начального и конечного индекса графа в список индексов, кот. будут из себ€ представл€ть путь
	}

	//AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA

	/*LONG dXLeft = min(fromPoint.x,toPoint.x);
	LONG dXRight = max(fromPoint.x, toPoint.x);
	LONG dYTop = min(fromPoint.y, toPoint.y);
	LONG dYBottom = max(fromPoint.y, toPoint.y);

	if (dXLeft != dXRight)
	{
		LONG dYDelta = (dYBottom - dYTop) / 2 + dYTop;
		AddPoint({ fromPoint.x, dYDelta });
		AddPoint({ toPoint.x, dYDelta });
	}

	AddPoint(toPoint);
*/
}

void CConnector::Draw(HDC dc)
{
	if (!m_aPath.empty())
	{
		HPEN hPen = nullptr;

		MoveToEx(dc, m_aPath[0].x, m_aPath[0].y, NULL);
		if (GetIntersected())
			hPen = CreatePen(PS_DOT, 1, 0x0000FF);
		else
			hPen = CreatePen(PS_SOLID, 1, m_nPenColor);

		HPEN oldPen = (HPEN)SelectObject(dc, hPen); 
		
		for (auto i = 1; i < m_aPath.size(); i++)
			LineTo(dc, m_aPath[i].x, m_aPath[i].y);

		SelectObject(dc, oldPen);
		
		DeleteObject(hPen);

	}
}

void CConnector::Clear(HDC dc)
{
	if (m_aPath.size() > 0)
	{
		HPEN hPen = CreatePen(PS_SOLID, 1, GetDCBrushColor(dc));
		HPEN oldPen = (HPEN)SelectObject(dc, hPen);

		MoveToEx(dc, m_aPath[0].x, m_aPath[0].y, NULL);
		for (auto i = 1; i < m_aPath.size(); i++)
			LineTo(dc, m_aPath[i].x, m_aPath[i].y);

		SelectObject(dc, oldPen);

		DeleteObject(hPen);
	}
}

bool CConnector::IsConnect(CRect* rect)
{
	for (CRect* itr : m_aRect)
		if (itr == rect)
			return true;
	return false;
}

bool CConnector::IsIntersect(CBaseObjects* pObject)
{
	if (pObject)
	{
		CRect* rect = (CRect*)pObject;
		if (rect == m_aRect[0] || rect == m_aRect[1])
			return false;
		if (m_aPath.size() > 1)
		{
			RECT r = rect->GetRect();
			for (auto i = 0; i < m_aPath.size() - 1; i++)
			{
				RECT o = {
					min(m_aPath[i].x,m_aPath[i + 1].x), min(m_aPath[i].y,m_aPath[i + 1].y),
					max(m_aPath[i].x,m_aPath[i + 1].x), max(m_aPath[i].y,m_aPath[i + 1].y),
				};

				if (::IsIntersect(o, r))
					return true;
			}
		}
	}
	return false;
}

