/********************************************************************************
 *	Copyright (C) 2010 by Stephen Allewell					*
 *	stephen@mirramar.adsl24.co.uk						*
 *										*
 *	This program is free software; you can redistribute it and/or modify	*
 *	it under the terms of the GNU General Public License as published by	*
 *	the Free Software Foundation; either version 2 of the License, or	*
 *	(at your option) any later version.					*
 ********************************************************************************/


#ifndef __BackgroundImage_H
#define __BackgroundImage_H


#include <QDataStream>
#include <QIcon>
#include <QImage>
#include <QRect>

#include <KUrl>


class BackgroundImage
{
	public:
		BackgroundImage();
		BackgroundImage(const KUrl &, const QRect &);

		const KUrl &url() const;
		const QRect &location() const;
		bool isVisible() const;
		bool isValid() const;
		const QImage &image() const;
		const QIcon &icon() const;

		void setUrl(const KUrl &);
		void setLocation(const QRect &);
		void setVisible(bool);
		void setStatus(bool);
		void setImage(const QImage &);
		void setIcon(const QIcon &);

		friend QDataStream &operator<<(QDataStream &, const BackgroundImage &);
		friend QDataStream &operator>>(QDataStream &, BackgroundImage &);

	private:
		static const int version = 100;

		KUrl	m_url;
		QRect	m_location;
		bool	m_visible;
		bool	m_status;
		QImage	m_image;
		QIcon	m_icon;
};


QDataStream &operator<<(QDataStream &, const BackgroundImage &);
QDataStream &operator>>(QDataStream &, BackgroundImage &);


#endif