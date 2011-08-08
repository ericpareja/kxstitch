/********************************************************************************
 *	Copyright (C) 2010 by Stephen Allewell					*
 *	stephen@mirramar.adsl24.co.uk						*
 *										*
 *	This program is free software; you can redistribute it and/or modify	*
 *	it under the terms of the GNU General Public License as published by	*
 *	the Free Software Foundation; either version 2 of the License, or	*
 *	(at your option) any later version.					*
 ********************************************************************************/


#include "Renderer.h"

#include <QPaintEngine>
#include <QPainter>
#include <QPen>

#include "Document.h"
#include "DocumentFloss.h"
#include "Stitch.h"


const Renderer::renderStitchCallPointer Renderer::renderStitchCallPointers[] =
{
	&Renderer::renderStitchesAsNone,
	&Renderer::renderStitchesAsStitches,
	&Renderer::renderStitchesAsBlackWhiteSymbols,
	&Renderer::renderStitchesAsColorSymbols,
	&Renderer::renderStitchesAsColorBlocks,
	&Renderer::renderStitchesAsColorBlocksSymbols,
};

const Renderer::renderBackstitchCallPointer Renderer::renderBackstitchCallPointers[] =
{
	&Renderer::renderBackstitchesAsNone,
	&Renderer::renderBackstitchesAsColorLines,
	&Renderer::renderBackstitchesAsBlackWhiteSymbols,
};

const Renderer::renderKnotCallPointer Renderer::renderKnotCallPointers[] =
{
	&Renderer::renderKnotsAsNone,
	&Renderer::renderKnotsAsColorBlocks,
	&Renderer::renderKnotsAsColorSymbols,
	&Renderer::renderKnotsAsBlackWhiteSymbols,
};


Renderer::Renderer()
	:	m_renderStitchesAs(Configuration::EnumRenderer_RenderStitchesAs::None),
		m_renderBackstitchesAs(Configuration::EnumRenderer_RenderBackstitchesAs::None),
		m_renderKnotsAs(Configuration::EnumRenderer_RenderKnotsAs::None)
{
}


Renderer::~Renderer()
{
}


void Renderer::setCellSize(double cellWidth, double cellHeight)
{
	if ((cellWidth != m_cellWidth) || (cellHeight != m_cellHeight))
	{
		double halfWidth = cellWidth/2;
		double halfHeight = cellHeight/2;
		double thirdWidth = cellWidth/3;
		double thirdHeight = cellHeight/3;

		m_renderCell = QRectF(0, 0, cellWidth, cellHeight);
		m_renderTLCell = QRectF(0, 0, halfWidth, halfHeight);
		m_renderTL3Cell = QRectF(0, 0, cellWidth-thirdWidth, cellHeight-thirdHeight);
		m_renderTRCell = QRectF(halfWidth, 0, halfWidth, halfHeight);
		m_renderTR3Cell = QRectF(thirdWidth, 0, cellWidth-thirdWidth, cellHeight-thirdHeight);
		m_renderBLCell = QRectF(0, halfHeight, halfWidth, halfHeight);
		m_renderBL3Cell = QRectF(0, thirdHeight, cellWidth-thirdWidth, cellHeight-thirdHeight);
		m_renderBRCell = QRectF(halfWidth, halfHeight, halfWidth, halfHeight);
		m_renderBR3Cell = QRectF(thirdWidth, thirdHeight, cellWidth-thirdWidth, cellHeight-thirdHeight);

		m_renderTLQ << QPointF(0, 0) << QPointF(halfWidth, 0) << QPointF(0, halfHeight);
		m_renderTRQ << QPointF(halfWidth, 0) << QPointF(cellWidth, 0) << QPointF(cellWidth, halfHeight);
		m_renderBLQ << QPointF(0, halfHeight) << QPointF(halfWidth, cellHeight) << QPointF(0, cellHeight);
		m_renderBRQ << QPointF(cellWidth, halfHeight) << QPointF(cellWidth, cellHeight) << QPointF(halfWidth, cellHeight);
		m_renderBLTRH << QPointF(0, cellHeight) << QPointF(0, halfHeight) << QPointF(halfWidth, 0) << QPointF(cellWidth, 0) << QPointF(cellWidth, halfHeight) << QPointF(halfWidth, cellHeight);
		m_renderTLBRH << QPointF(0, 0) << QPointF(halfWidth, 0) << QPointF(cellWidth, halfHeight) << QPointF(cellWidth, cellHeight) << QPointF(halfWidth, cellHeight) << QPointF(0, halfWidth);
		m_renderTL3Q << QPointF(0, 0) << QPointF(cellWidth, 0) << QPointF(cellWidth, halfHeight) << QPointF(halfWidth, cellHeight) << QPointF(0, cellHeight);
		m_renderTR3Q << QPointF(0, 0) << QPointF(cellWidth, 0) << QPointF(cellWidth, cellHeight) << QPointF(halfWidth, cellHeight) << QPointF(0, halfHeight);
		m_renderBL3Q << QPointF(0, 0) << QPointF(halfWidth, 0) << QPointF(cellWidth, halfHeight) << QPointF(cellWidth, cellHeight) << QPointF(0, cellHeight);
		m_renderBR3Q << QPointF(halfWidth, 0) << QPointF(cellWidth, 0) << QPointF(cellWidth, cellHeight) << QPointF(0, cellHeight) << QPointF(0, halfHeight);

		m_renderFontSize = std::min(cellWidth, cellHeight)*3/4;
		m_renderQtrFontSize = m_renderFontSize/2;
		m_render3QtrFontSize = m_renderFontSize*3/4;

		m_cellWidth = cellWidth;
		m_cellHeight = cellHeight;
	}
}


void Renderer::setPatternRect(const QRect &rect)
{
	m_patternRect = rect;
}


void Renderer::setRenderStitchesAs(Configuration::EnumRenderer_RenderStitchesAs::type renderStitchesAs)
{
	m_renderStitchesAs = renderStitchesAs;
}


void Renderer::setRenderBackstitchesAs(Configuration::EnumRenderer_RenderBackstitchesAs::type renderBackstitchesAs)
{
	m_renderBackstitchesAs = renderBackstitchesAs;
}


void Renderer::setRenderKnotsAs(Configuration::EnumRenderer_RenderKnotsAs::type renderKnotsAs)
{
	m_renderKnotsAs = renderKnotsAs;
}


void Renderer::setPaintDeviceArea(const QRectF &rect)
{
	m_paintDeviceArea = rect;
}


void Renderer::render(QPainter *painter,
		      Document *document,
		      const QRect &updateRectangle,
		      const QList<int> &layerOrder,
		      const QList<int> &visibleLayers,
		      bool renderGrid,
		      bool renderStitches,
		      bool renderBackstitches,
		      bool renderKnots,
		      int colorHilight)
{
	kDebug() << "paint device type" << painter->device()->paintEngine()->type();

	bool paintDeviceIsScreen = (painter->device()->paintEngine()->type() == QPaintEngine::X11); // test for other types

	painter->save();
	m_painter = painter;
	m_document = document;
	m_hilight = colorHilight;

	painter->drawRect(m_paintDeviceArea);

	double l = m_paintDeviceArea.left();
	double t = m_paintDeviceArea.top();
	double w = m_paintDeviceArea.width();
	double h = m_paintDeviceArea.height();
	double dx = w / m_patternRect.width();
	double dy = h / m_patternRect.height();

	painter->translate(l, t);

	if (renderGrid)
	{
		int cellHorizontalGrouping = m_document->property("cellHorizontalGrouping").toInt();
		int cellVerticalGrouping = m_document->property("cellVerticalGrouping").toInt();

		for (int x = 0 ; x < m_patternRect.width() ; x++)
		{
			if (x % cellHorizontalGrouping)
				painter->setPen(paintDeviceIsScreen?QPen(Qt::lightGray):QPen(Qt::black, 0.2));
			else
				painter->setPen(paintDeviceIsScreen?QPen(Qt::darkGray):QPen(Qt::black, 1.0));
			painter->drawLine(x*dx, 0, x*dx, h);
		}

		for (int y = 0 ; y < m_patternRect.height() ; y++)
		{
			if (y % cellVerticalGrouping)
				painter->setPen(paintDeviceIsScreen?QPen(Qt::lightGray):QPen(Qt::black, 0.2));
			else
				painter->setPen(paintDeviceIsScreen?QPen(Qt::darkGray):QPen(Qt::black, 1.0));
			painter->drawLine(0, y*dy, w, y*dy);
		}
	}

	int leftCell = std::max(0, int((updateRectangle.left()-l)/dx));
	int topCell = std::max(0, int((updateRectangle.top()-t)/dy));
	int rightCell = std::min(m_patternRect.width(), int((updateRectangle.right()-l)/dx));
	int bottomCell = std::min(m_patternRect.height(), int((updateRectangle.bottom()-t)/dx));

	m_renderFont = m_painter->font();
	m_renderFont.setFamily(Configuration::editor_SymbolFont());
	m_renderFont.setPointSize(m_renderFontSize);
	m_renderQtrFont = m_renderFont;
	m_renderQtrFont.setPointSize(m_renderQtrFontSize);
	m_render3QtrFont = m_renderFont;
	m_render3QtrFont.setPointSize(m_render3QtrFontSize);

	QRect snapUpdateRectangle(leftCell*2, topCell*2, (rightCell-leftCell)*2, (bottomCell-topCell)*2);

	QListIterator<int> layerIterator(layerOrder);
	while (layerIterator.hasNext())
	{
		int layer = layerIterator.next();
		if (visibleLayers.contains(layer))
		{
			if (renderStitches)
			{
				for (int y =  topCell ; y <= bottomCell ; y++)
				{
					for (int x = leftCell ; x <= rightCell ; x++)
					{
						StitchQueue *queue = m_document->stitchData().stitchQueueAt(layer, QPoint(x, y));
						if (queue)
						{
							double xpos = x*m_cellWidth;
							double ypos = y*m_cellHeight;
							m_painter->resetTransform();
							m_painter->translate(xpos+l, ypos+t);
							(this->*renderStitchCallPointers[m_renderStitchesAs])(queue);
						}
					}
				}
			}
			m_painter->resetTransform();
			m_painter->translate(l, t);
			// process backstitches
			if (renderBackstitches)
			{
				QListIterator<Backstitch *> backstitchIterator = m_document->stitchData().backstitchIterator(layer);
				while (backstitchIterator.hasNext())
				{
					Backstitch *backstitch = backstitchIterator.next();
					(this->*renderBackstitchCallPointers[m_renderBackstitchesAs])(backstitch);
				}
			}
			// process knots
			if (renderKnots)
			{
				QListIterator<Knot *> knotIterator = m_document->stitchData().knotIterator(layer);
				while (knotIterator.hasNext())
				{
					Knot *knot = knotIterator.next();
					if ((m_hilight == -1) || (m_hilight = knot->colorIndex))
					{
						(this->*renderKnotCallPointers[m_renderKnotsAs])(knot);
					}
				}
			}
		}
	}
	painter->restore();
}


void Renderer::renderStitchesAsNone(StitchQueue *)
{
}


void Renderer::renderStitchesAsStitches(StitchQueue *stitchQueue)
{
	QPen pen;
	StitchQueue::const_iterator it;
	for (it = stitchQueue->begin() ; it != stitchQueue->end() ; ++it)
	{
		Stitch *stitch = *it;
		DocumentFloss *documentFloss = m_document->documentPalette().flosses()[stitch->colorIndex];
		if ((m_hilight == -1) || (stitch->colorIndex == m_hilight))
		{
			pen.setColor(documentFloss->flossColor());
			pen.setWidth(documentFloss->stitchStrands());
		}
		else
		{
			pen.setColor(Qt::lightGray);
			pen.setWidth(0);
		}

		m_painter->setPen(pen);

		switch (stitch->type)
		{
			case Stitch::Delete:
				break;

			case Stitch::TLQtr:
				m_painter->drawLine(m_renderCell.topLeft(), m_renderCell.center());
				break;

			case Stitch::TRQtr:
				m_painter->drawLine(m_renderCell.topRight(), m_renderCell.center());
				break;

			case Stitch::BLQtr:
				m_painter->drawLine(m_renderCell.bottomLeft(), m_renderCell.center());
				break;

			case Stitch::BTHalf:
				m_painter->drawLine(m_renderCell.bottomLeft(), m_renderCell.topRight());
				break;

			case Stitch::TL3Qtr:
				m_painter->drawLine(m_renderCell.bottomLeft(), m_renderCell.topRight());
				m_painter->drawLine(m_renderCell.topLeft(), m_renderCell.center());
				break;

			case Stitch::BRQtr:
				m_painter->drawLine(m_renderCell.bottomRight(), m_renderCell.center());
				break;

			case Stitch::TBHalf:
				m_painter->drawLine(m_renderCell.topLeft(), m_renderCell.bottomRight());
				break;

			case Stitch::TR3Qtr:
				m_painter->drawLine(m_renderCell.topLeft(), m_renderCell.bottomRight());
				m_painter->drawLine(m_renderCell.topRight(), m_renderCell.center());
				break;

			case Stitch::BL3Qtr:
				m_painter->drawLine(m_renderCell.topLeft(), m_renderCell.bottomRight());
				m_painter->drawLine(m_renderCell.bottomLeft(), m_renderCell.center());
				break;

			case Stitch::BR3Qtr:
				m_painter->drawLine(m_renderCell.bottomLeft(), m_renderCell.topRight());
				m_painter->drawLine(m_renderCell.bottomRight(), m_renderCell.center());
				break;

			case Stitch::Full:
				m_painter->drawLine(m_renderCell.topLeft(), m_renderCell.bottomRight());
				m_painter->drawLine(m_renderCell.bottomLeft(), m_renderCell.topRight());
				break;

			case Stitch::TLSmallHalf:
				m_painter->drawLine(m_renderCell.left(), m_renderCell.center().y(), m_renderCell.center().x(), m_renderCell.top());
				break;

			case Stitch::TRSmallHalf:
				m_painter->drawLine(m_renderCell.center().x(), m_renderCell.top(), m_renderCell.right(), m_renderCell.center().y());
				break;

			case Stitch::BLSmallHalf:
				m_painter->drawLine(m_renderCell.left(), m_renderCell.center().y(), m_renderCell.center().x(), m_renderCell.bottom());
				break;

			case Stitch::BRSmallHalf:
				m_painter->drawLine(m_renderCell.center().x(), m_renderCell.bottom(), m_renderCell.right(), m_renderCell.center().y());
				break;

			case Stitch::TLSmallFull:
				m_painter->drawLine(m_renderCell.left(), m_renderCell.center().y(), m_renderCell.center().x(), m_renderCell.top());
				m_painter->drawLine(m_renderCell.topLeft(), m_renderCell.center());
				break;

			case Stitch::TRSmallFull:
				m_painter->drawLine(m_renderCell.center().x(), m_renderCell.top(), m_renderCell.right(), m_renderCell.center().y());
				m_painter->drawLine(m_renderCell.topRight(), m_renderCell.center());
				break;

			case Stitch::BLSmallFull:
				m_painter->drawLine(m_renderCell.left(), m_renderCell.center().y(), m_renderCell.center().x(), m_renderCell.bottom());
				m_painter->drawLine(m_renderCell.bottomLeft(), m_renderCell.center());
				break;

			case Stitch::BRSmallFull:
				m_painter->drawLine(m_renderCell.center().x(), m_renderCell.bottom(), m_renderCell.right(), m_renderCell.center().y());
				m_painter->drawLine(m_renderCell.bottomRight(), m_renderCell.center());
				break;
		}
	}
}


void Renderer::renderStitchesAsBlackWhiteSymbols(StitchQueue *stitchQueue)
{
	QPen symbolPen;
	QPen outlinePen(Qt::lightGray);
	outlinePen.setWidth(0);
	StitchQueue::const_iterator it;
	for (it = stitchQueue->begin() ; it != stitchQueue->end() ; ++it)
	{
		Stitch *stitch = *it;
		DocumentFloss *documentFloss = m_document->documentPalette().flosses()[stitch->colorIndex];
		if ((m_hilight == -1) || (stitch->colorIndex == m_hilight))
		{
			symbolPen.setColor(Qt::black);
			symbolPen.setWidth(documentFloss->stitchStrands());
		}
		else
		{
			symbolPen.setColor(Qt::lightGray);
			symbolPen.setWidth(0);
		}

		switch (stitch->type)
		{
			case Stitch::Delete:
				break;

			case Stitch::TLQtr:
				m_painter->setFont(m_renderQtrFont);
				m_painter->setPen(outlinePen);
				m_painter->drawLine(m_renderTLCell.topLeft(), m_renderTLCell.bottomRight());
				m_painter->setPen(symbolPen);
				m_painter->drawText(m_renderTLCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::TRQtr:
				m_painter->setFont(m_renderQtrFont);
				m_painter->setPen(outlinePen);
				m_painter->drawLine(m_renderTRCell.bottomLeft(), m_renderTRCell.topRight());
				m_painter->setPen(symbolPen);
				m_painter->drawText(m_renderTRCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::BLQtr:
				m_painter->setFont(m_renderQtrFont);
				m_painter->setPen(outlinePen);
				m_painter->drawLine(m_renderBLCell.bottomLeft(), m_renderBLCell.topRight());
				m_painter->setPen(symbolPen);
				m_painter->drawText(m_renderBLCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::BTHalf:
				m_painter->setFont(m_render3QtrFont);
				m_painter->setPen(outlinePen);
				m_painter->drawLine(m_renderCell.bottomLeft(), m_renderCell.topRight());
				m_painter->setPen(symbolPen);
				m_painter->drawText(m_renderCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::TL3Qtr:
				m_painter->setFont(m_render3QtrFont);
				m_painter->setPen(outlinePen);
				m_painter->drawLine(m_renderTL3Cell.bottomLeft(), m_renderTL3Cell.topRight());
				m_painter->drawLine(m_renderTL3Cell.center(), m_renderTL3Cell.topLeft());
				m_painter->setPen(symbolPen);
				m_painter->drawText(m_renderTL3Cell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::BRQtr:
				m_painter->setFont(m_renderQtrFont);
				m_painter->setPen(outlinePen);
				m_painter->drawLine(m_renderBRCell.topLeft(), m_renderBRCell.bottomRight());
				m_painter->setPen(symbolPen);
				m_painter->drawText(m_renderBRCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::TBHalf:
				m_painter->setFont(m_render3QtrFont);
				m_painter->setPen(outlinePen);
				m_painter->drawLine(m_renderCell.topLeft(), m_renderCell.bottomRight());
				m_painter->setPen(symbolPen);
				m_painter->drawText(m_renderCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::TR3Qtr:
				m_painter->setFont(m_render3QtrFont);
				m_painter->setPen(outlinePen);
				m_painter->drawLine(m_renderTR3Cell.topLeft(), m_renderTR3Cell.bottomRight());
				m_painter->drawLine(m_renderTR3Cell.center(), m_renderTR3Cell.topRight());
				m_painter->setPen(symbolPen);
				m_painter->drawText(m_renderTR3Cell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::BL3Qtr:
				m_painter->setFont(m_render3QtrFont);
				m_painter->setPen(outlinePen);
				m_painter->drawLine(m_renderBL3Cell.topLeft(), m_renderBL3Cell.bottomRight());
				m_painter->drawLine(m_renderBL3Cell.center(), m_renderBL3Cell.bottomLeft());
				m_painter->setPen(symbolPen);
				m_painter->drawText(m_renderBL3Cell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::BR3Qtr:
				m_painter->setFont(m_render3QtrFont);
				m_painter->setPen(outlinePen);
				m_painter->drawLine(m_renderBR3Cell.bottomLeft(), m_renderBR3Cell.topRight());
				m_painter->drawLine(m_renderBR3Cell.center(), m_renderBR3Cell.bottomRight());
				m_painter->setPen(symbolPen);
				m_painter->drawText(m_renderBR3Cell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::Full:
				m_painter->setFont(m_renderFont);
				m_painter->setPen(symbolPen);
				m_painter->drawText(m_renderCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::TLSmallHalf:
				m_painter->setFont(m_renderQtrFont);
				m_painter->setPen(outlinePen);
				m_painter->drawLine(m_renderTLCell.bottomLeft(), m_renderTLCell.topRight());
				m_painter->setPen(symbolPen);
				m_painter->drawText(m_renderTLCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::TRSmallHalf:
				m_painter->setFont(m_renderQtrFont);
				m_painter->setPen(outlinePen);
				m_painter->drawLine(m_renderTRCell.topLeft(), m_renderTRCell.bottomRight());
				m_painter->setPen(symbolPen);
				m_painter->drawText(m_renderTRCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::BLSmallHalf:
				m_painter->setFont(m_renderQtrFont);
				m_painter->setPen(outlinePen);
				m_painter->drawLine(m_renderBLCell.topLeft(), m_renderBLCell.bottomRight());
				m_painter->setPen(symbolPen);
				m_painter->drawText(m_renderBLCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::BRSmallHalf:
				m_painter->setFont(m_renderQtrFont);
				m_painter->setPen(outlinePen);
				m_painter->drawLine(m_renderBRCell.bottomLeft(), m_renderBRCell.topRight());
				m_painter->setPen(symbolPen);
				m_painter->drawText(m_renderBRCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::TLSmallFull:
				m_painter->setFont(m_renderQtrFont);
				m_painter->setPen(outlinePen);
				m_painter->drawLine(m_renderTLCell.topLeft(), m_renderTLCell.bottomRight());
				m_painter->drawLine(m_renderTLCell.bottomLeft(), m_renderTLCell.topRight());
				m_painter->setPen(symbolPen);
				m_painter->drawText(m_renderTLCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::TRSmallFull:
				m_painter->setFont(m_renderQtrFont);
				m_painter->setPen(outlinePen);
				m_painter->drawLine(m_renderTRCell.topLeft(), m_renderTRCell.bottomRight());
				m_painter->drawLine(m_renderTRCell.bottomLeft(), m_renderTRCell.topRight());
				m_painter->setPen(symbolPen);
				m_painter->drawText(m_renderTRCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::BLSmallFull:
				m_painter->setFont(m_renderQtrFont);
				m_painter->setPen(outlinePen);
				m_painter->drawLine(m_renderBLCell.topLeft(), m_renderBLCell.bottomRight());
				m_painter->drawLine(m_renderBLCell.bottomLeft(), m_renderBLCell.topRight());
				m_painter->setPen(symbolPen);
				m_painter->drawText(m_renderBLCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::BRSmallFull:
				m_painter->setFont(m_renderQtrFont);
				m_painter->setPen(outlinePen);
				m_painter->drawLine(m_renderBRCell.topLeft(), m_renderBRCell.bottomRight());
				m_painter->drawLine(m_renderBRCell.bottomLeft(), m_renderBRCell.topRight());
				m_painter->setPen(symbolPen);
				m_painter->drawText(m_renderBRCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;
		}
	}
}


void Renderer::renderStitchesAsColorSymbols(StitchQueue *stitchQueue)
{
	QPen pen;
	StitchQueue::const_iterator it;
	for (it = stitchQueue->begin() ; it != stitchQueue->end() ; ++it)
	{
		Stitch *stitch = *it;
		DocumentFloss *documentFloss = m_document->documentPalette().flosses()[stitch->colorIndex];
		if ((m_hilight == -1) || (stitch->colorIndex == m_hilight))
		{
			pen.setColor(documentFloss->flossColor());
		}
		else
		{
			pen.setColor(Qt::lightGray);
		}
		m_painter->setPen(pen);

		switch (stitch->type)
		{
			case Stitch::Delete:
				break;

			case Stitch::TLQtr:
				m_painter->setFont(m_renderQtrFont);
				m_painter->drawLine(m_renderTLCell.topLeft(), m_renderTLCell.bottomRight());
				m_painter->drawText(m_renderTLCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::TRQtr:
				m_painter->setFont(m_renderQtrFont);
				m_painter->drawLine(m_renderTRCell.bottomLeft(), m_renderTRCell.topRight());
				m_painter->drawText(m_renderTRCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::BLQtr:
				m_painter->setFont(m_renderQtrFont);
				m_painter->drawLine(m_renderBLCell.bottomLeft(), m_renderBLCell.topRight());
				m_painter->drawText(m_renderBLCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::BTHalf:
				m_painter->setFont(m_render3QtrFont);
				m_painter->drawLine(m_renderCell.bottomLeft(), m_renderCell.topRight());
				m_painter->drawText(m_renderCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::TL3Qtr:
				m_painter->setFont(m_render3QtrFont);
				m_painter->drawLine(m_renderTL3Cell.bottomLeft(), m_renderTL3Cell.topRight());
				m_painter->drawLine(m_renderTL3Cell.center(), m_renderTL3Cell.topLeft());
				m_painter->drawText(m_renderTL3Cell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::BRQtr:
				m_painter->setFont(m_renderQtrFont);
				m_painter->drawLine(m_renderBRCell.topLeft(), m_renderBRCell.bottomRight());
				m_painter->drawText(m_renderBRCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::TBHalf:
				m_painter->setFont(m_render3QtrFont);
				m_painter->drawLine(m_renderCell.topLeft(), m_renderCell.bottomRight());
				m_painter->drawText(m_renderCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::TR3Qtr:
				m_painter->setFont(m_render3QtrFont);
				m_painter->drawLine(m_renderTR3Cell.topLeft(), m_renderTR3Cell.bottomRight());
				m_painter->drawLine(m_renderTR3Cell.center(), m_renderTR3Cell.topRight());
				m_painter->drawText(m_renderTR3Cell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::BL3Qtr:
				m_painter->setFont(m_render3QtrFont);
				m_painter->drawLine(m_renderBL3Cell.topLeft(), m_renderBL3Cell.bottomRight());
				m_painter->drawLine(m_renderBL3Cell.center(), m_renderBL3Cell.bottomLeft());
				m_painter->drawText(m_renderBL3Cell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::BR3Qtr:
				m_painter->setFont(m_render3QtrFont);
				m_painter->drawLine(m_renderBR3Cell.bottomLeft(), m_renderBR3Cell.topRight());
				m_painter->drawLine(m_renderBR3Cell.center(), m_renderBR3Cell.bottomRight());
				m_painter->drawText(m_renderBR3Cell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::Full:
				m_painter->setFont(m_renderFont);
				m_painter->drawText(m_renderCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::TLSmallHalf:
				m_painter->setFont(m_renderQtrFont);
				m_painter->drawLine(m_renderTLCell.bottomLeft(), m_renderTLCell.topRight());
				m_painter->drawText(m_renderTLCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::TRSmallHalf:
				m_painter->setFont(m_renderQtrFont);
				m_painter->drawLine(m_renderTRCell.topLeft(), m_renderTRCell.bottomRight());
				m_painter->drawText(m_renderTRCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::BLSmallHalf:
				m_painter->setFont(m_renderQtrFont);
				m_painter->drawLine(m_renderBLCell.topLeft(), m_renderBLCell.bottomRight());
				m_painter->drawText(m_renderBLCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::BRSmallHalf:
				m_painter->setFont(m_renderQtrFont);
				m_painter->drawLine(m_renderBRCell.bottomLeft(), m_renderBRCell.topRight());
				m_painter->drawText(m_renderBRCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::TLSmallFull:
				m_painter->setFont(m_renderQtrFont);
				m_painter->drawLine(m_renderTLCell.topLeft(), m_renderTLCell.bottomRight());
				m_painter->drawLine(m_renderTLCell.bottomLeft(), m_renderTLCell.topRight());
				m_painter->drawText(m_renderTLCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::TRSmallFull:
				m_painter->setFont(m_renderQtrFont);
				m_painter->drawLine(m_renderTRCell.topLeft(), m_renderTRCell.bottomRight());
				m_painter->drawLine(m_renderTRCell.bottomLeft(), m_renderTRCell.topRight());
				m_painter->drawText(m_renderTRCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::BLSmallFull:
				m_painter->setFont(m_renderQtrFont);
				m_painter->drawLine(m_renderBLCell.topLeft(), m_renderBLCell.bottomRight());
				m_painter->drawLine(m_renderBLCell.bottomLeft(), m_renderBLCell.topRight());
				m_painter->drawText(m_renderBLCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::BRSmallFull:
				m_painter->setFont(m_renderQtrFont);
				m_painter->drawLine(m_renderBRCell.topLeft(), m_renderBRCell.bottomRight());
				m_painter->drawLine(m_renderBRCell.bottomLeft(), m_renderBRCell.topRight());
				m_painter->drawText(m_renderBRCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;
		}
	}
}


void Renderer::renderStitchesAsColorBlocks(StitchQueue *stitchQueue)
{
	StitchQueue::const_iterator it;
	for (it = stitchQueue->begin() ; it != stitchQueue->end() ; ++it)
	{
		Stitch *stitch = *it;
		DocumentFloss *documentFloss = m_document->documentPalette().flosses()[stitch->colorIndex];
		QBrush brush(Qt::SolidPattern);
		QPen pen;
		if ((m_hilight == -1) || (stitch->colorIndex == m_hilight))
		{
			brush.setColor(documentFloss->flossColor());
			pen.setColor(documentFloss->flossColor());
		}
		else
		{
			brush.setColor(Qt::lightGray);
			pen.setColor(Qt::lightGray);
		}
		m_painter->setBrush(brush);
		m_painter->setPen(pen);

		switch (stitch->type)
		{
			case Stitch::Delete:
				break;

			case Stitch::TLQtr:
				m_painter->drawPolygon(m_renderTLQ);
				break;

			case Stitch::TRQtr:
				m_painter->drawPolygon(m_renderTRQ);
				break;

			case Stitch::BLQtr:
				m_painter->drawPolygon(m_renderBLQ);
				break;

			case Stitch::BTHalf:
				m_painter->drawPolygon(m_renderBLTRH);
				break;

			case Stitch::TL3Qtr:
				m_painter->drawPolygon(m_renderTL3Q);
				break;

			case Stitch::BRQtr:
				m_painter->drawPolygon(m_renderBRQ);
				break;

			case Stitch::TBHalf:
				m_painter->drawPolygon(m_renderTLBRH);
				break;

			case Stitch::TR3Qtr:
				m_painter->drawPolygon(m_renderTR3Q);
				break;

			case Stitch::BL3Qtr:
				m_painter->drawPolygon(m_renderBL3Q);
				break;

			case Stitch::BR3Qtr:
				m_painter->drawPolygon(m_renderBR3Q);
				break;

			case Stitch::Full:
				m_painter->fillRect(m_renderCell, brush);
				break;

			case Stitch::TLSmallHalf:
				break;

			case Stitch::TRSmallHalf:
				break;

			case Stitch::BLSmallHalf:
				break;

			case Stitch::BRSmallHalf:
				break;

			case Stitch::TLSmallFull:
				m_painter->fillRect(m_renderTLCell, brush);
				break;

			case Stitch::TRSmallFull:
				m_painter->fillRect(m_renderTRCell, brush);
				break;

			case Stitch::BLSmallFull:
				m_painter->fillRect(m_renderBLCell, brush);
				break;

			case Stitch::BRSmallFull:
				m_painter->fillRect(m_renderBRCell, brush);
				break;
		}
	}
}


void Renderer::renderStitchesAsColorBlocksSymbols(StitchQueue *stitchQueue)
{
	StitchQueue::const_iterator it;
	for (it = stitchQueue->begin() ; it != stitchQueue->end() ; ++it)
	{
		Stitch *stitch = *it;
		DocumentFloss *documentFloss = m_document->documentPalette().flosses()[stitch->colorIndex];
		QPen textPen;
		QPen outlinePen;
		QBrush brush(Qt::SolidPattern);
		if ((m_hilight == -1) || (stitch->colorIndex == m_hilight))
		{
			textPen.setColor((qGray(documentFloss->flossColor().rgb()) < 128)?Qt::white:Qt::black);
			outlinePen.setColor(documentFloss->flossColor());
			brush.setColor(documentFloss->flossColor());
		}
		else
		{
			outlinePen.setColor(Qt::lightGray);
			brush.setColor(Qt::lightGray);
			textPen.setColor(Qt::darkGray);
		}
		m_painter->setPen(outlinePen);
		m_painter->setBrush(brush);

		switch (stitch->type)
		{
			case Stitch::Delete:
				break;

			case Stitch::TLQtr:
				m_painter->drawPolygon(m_renderTLQ);
				m_painter->setPen(textPen);
				m_painter->setFont(m_renderQtrFont);
				m_painter->drawText(m_renderTLCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::TRQtr:
				m_painter->drawPolygon(m_renderTRQ);
				m_painter->setPen(textPen);
				m_painter->setFont(m_renderQtrFont);
				m_painter->drawText(m_renderTRCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::BLQtr:
				m_painter->drawPolygon(m_renderBLQ);
				m_painter->setFont(m_renderQtrFont);
				m_painter->setPen(textPen);
				m_painter->drawText(m_renderBLCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::BTHalf:
				m_painter->drawPolygon(m_renderBLTRH);
				m_painter->setPen(textPen);
				m_painter->setFont(m_render3QtrFont);
				m_painter->drawText(m_renderCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::TL3Qtr:
				m_painter->drawPolygon(m_renderTL3Q);
				m_painter->setPen(textPen);
				m_painter->setFont(m_render3QtrFont);
				m_painter->drawText(m_renderTL3Cell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::BRQtr:
				m_painter->drawPolygon(m_renderBRQ);
				m_painter->setPen(textPen);
				m_painter->setFont(m_renderQtrFont);
				m_painter->drawText(m_renderBRCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::TBHalf:
				m_painter->drawPolygon(m_renderTLBRH);
				m_painter->setPen(textPen);
				m_painter->setFont(m_render3QtrFont);
				m_painter->drawText(m_renderCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::TR3Qtr:
				m_painter->drawPolygon(m_renderTR3Q);
				m_painter->setPen(textPen);
				m_painter->setFont(m_render3QtrFont);
				m_painter->drawText(m_renderTR3Cell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::BL3Qtr:
				m_painter->drawPolygon(m_renderBL3Q);
				m_painter->setPen(textPen);
				m_painter->setFont(m_render3QtrFont);
				m_painter->drawText(m_renderBL3Cell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::BR3Qtr:
				m_painter->drawPolygon(m_renderBR3Q);
				m_painter->setPen(textPen);
				m_painter->setFont(m_render3QtrFont);
				m_painter->drawText(m_renderBR3Cell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::Full:
				m_painter->fillRect(m_renderCell, brush);
				m_painter->setPen(textPen);
				m_painter->setFont(m_renderFont);
				m_painter->drawText(m_renderCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::TLSmallHalf:
				m_painter->setPen(textPen);
				m_painter->setFont(m_renderQtrFont);
				m_painter->drawText(m_renderTLCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::TRSmallHalf:
				m_painter->setPen(textPen);
				m_painter->setFont(m_renderQtrFont);
				m_painter->drawText(m_renderTRCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::BLSmallHalf:
				m_painter->setPen(textPen);
				m_painter->setFont(m_renderQtrFont);
				m_painter->drawText(m_renderBLCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::BRSmallHalf:
				m_painter->setPen(textPen);
				m_painter->setFont(m_renderQtrFont);
				m_painter->drawText(m_renderBRCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::TLSmallFull:
				m_painter->fillRect(m_renderTLCell, brush);
				m_painter->setPen(textPen);
				m_painter->setFont(m_renderQtrFont);
				m_painter->drawText(m_renderTLCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::TRSmallFull:
				m_painter->fillRect(m_renderTRCell, brush);
				m_painter->setPen(textPen);
				m_painter->setFont(m_renderQtrFont);
				m_painter->drawText(m_renderTRCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::BLSmallFull:
				m_painter->fillRect(m_renderBLCell, brush);
				m_painter->setPen(textPen);
				m_painter->setFont(m_renderQtrFont);
				m_painter->drawText(m_renderBLCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;

			case Stitch::BRSmallFull:
				m_painter->fillRect(m_renderBRCell, brush);
				m_painter->setPen(textPen);
				m_painter->setFont(m_renderQtrFont);
				m_painter->drawText(m_renderBRCell, Qt::AlignCenter, documentFloss->stitchSymbol());
				break;
		}
	}
}


void Renderer::renderBackstitchesAsNone(Backstitch *backstitch)
{
}


void Renderer::renderBackstitchesAsColorLines(Backstitch *backstitch)
{
	QPoint start((backstitch->start.x()*m_cellWidth)/2, (backstitch->start.y()*m_cellHeight)/2);
	QPoint end((backstitch->end.x()*m_cellWidth)/2, (backstitch->end.y()*m_cellHeight)/2);
	DocumentFloss *documentFloss = m_document->documentPalette().flosses()[backstitch->colorIndex];

	QPen pen;
	if ((m_hilight == -1) || (backstitch->colorIndex == m_hilight))
	{
		pen.setColor(documentFloss->flossColor());
		pen.setWidth(documentFloss->backstitchStrands());
	}
	else
	{
		pen.setColor(Qt::lightGray);
		pen.setWidth(0);
	}

	m_painter->setPen(pen);
	m_painter->drawLine(start, end);
}


void Renderer::renderBackstitchesAsBlackWhiteSymbols(Backstitch *backstitch)
{
	QPoint start((backstitch->start.x()*m_cellWidth)/2, (backstitch->start.y()*m_cellHeight)/2);
	QPoint end((backstitch->end.x()*m_cellWidth)/2, (backstitch->end.y()*m_cellHeight)/2);
	DocumentFloss *documentFloss = m_document->documentPalette().flosses()[backstitch->colorIndex];

	QPen pen;
	pen.setStyle(documentFloss->backstitchSymbol());
	if ((m_hilight == -1) || (backstitch->colorIndex == m_hilight))
	{
		pen.setColor(Qt::black);
		pen.setWidth(documentFloss->backstitchStrands());
	}
	else
	{
		pen.setColor(Qt::lightGray);
		pen.setWidth(0);
	}

	m_painter->setPen(pen);
	m_painter->drawLine(start, end);
}


void Renderer::renderKnotsAsNone(Knot *knot)
{
}


void Renderer::renderKnotsAsColorBlocks(Knot *knot)
{
}


void Renderer::renderKnotsAsColorSymbols(Knot *knot)
{
}


void Renderer::renderKnotsAsBlackWhiteSymbols(Knot *knot)
{
}
