/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.ui.eclipse.alarmviewer;

import java.text.DateFormat;

import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.Severity;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.alarmviewer.widgets.AlarmList;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;


/**
 * Label provider for alarm list
 *
 */
public class AlarmListLabelProvider implements ITableLabelProvider
{
	// Constants
	private static final String[] stateText = { "Outstanding", "Acknowledged", "Terminated" };
	
	// Severity images
	private Image[] severityImages = new Image[5];
	private Image[] stateImages = new Image[3];
	
	/**
	 * Default constructor 
	 */
	public AlarmListLabelProvider()
	{
		super();

		severityImages[Severity.NORMAL] = StatusDisplayInfo.getStatusImage(Severity.NORMAL);
		severityImages[Severity.WARNING] = StatusDisplayInfo.getStatusImage(Severity.WARNING);
		severityImages[Severity.MINOR] = StatusDisplayInfo.getStatusImage(Severity.MINOR);
		severityImages[Severity.MAJOR] = StatusDisplayInfo.getStatusImage(Severity.MAJOR);
		severityImages[Severity.CRITICAL] = StatusDisplayInfo.getStatusImage(Severity.CRITICAL);

		stateImages[0] = Activator.getImageDescriptor("icons/outstanding.png").createImage();
		stateImages[1] = Activator.getImageDescriptor("icons/acknowledged.png").createImage();
		stateImages[2] = Activator.getImageDescriptor("icons/terminated.png").createImage();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		switch(columnIndex)
		{
			case AlarmList.COLUMN_SEVERITY:
				return severityImages[((Alarm)element).getCurrentSeverity()];
			case AlarmList.COLUMN_STATE:
				return stateImages[((Alarm)element).getState()];
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		switch(columnIndex)
		{
			case AlarmList.COLUMN_SEVERITY:
				return StatusDisplayInfo.getStatusText(((Alarm)element).getCurrentSeverity());
			case AlarmList.COLUMN_STATE:
				return stateText[((Alarm)element).getState()];
			case AlarmList.COLUMN_SOURCE:
				GenericObject object = ((NXCSession)ConsoleSharedData.getSession()).findObjectById(((Alarm)element).getSourceObjectId());
				return (object != null) ? object.getObjectName() : null;
			case AlarmList.COLUMN_MESSAGE:
				return ((Alarm)element).getMessage();
			case AlarmList.COLUMN_COUNT:
				return Integer.toString(((Alarm)element).getRepeatCount());
			case AlarmList.COLUMN_CREATED:
				return DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.SHORT).format(((Alarm)element).getCreationTime());
			case AlarmList.COLUMN_LASTCHANGE:
				return DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.SHORT).format(((Alarm)element).getLastChangeTime());
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#addListener(org.eclipse.jface.viewers.ILabelProviderListener)
	 */
	@Override
	public void addListener(ILabelProviderListener listener)
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#dispose()
	 */
	@Override
	public void dispose()
	{
		for(int i = 0; i < stateImages.length; i++)
			stateImages[i].dispose();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#isLabelProperty(java.lang.Object, java.lang.String)
	 */
	@Override
	public boolean isLabelProperty(Object element, String property)
	{
		return false;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#removeListener(org.eclipse.jface.viewers.ILabelProviderListener)
	 */
	@Override
	public void removeListener(ILabelProviderListener listener)
	{
	}
}
