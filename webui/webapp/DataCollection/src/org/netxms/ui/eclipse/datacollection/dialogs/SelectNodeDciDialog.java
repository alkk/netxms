/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.widgets.DciList;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Dialog for selecting DCI from specific node
 */
public class SelectNodeDciDialog extends Dialog
{
	private static final long serialVersionUID = 1L;

	private long nodeId;
	private DciList dciList;
	private DciValue selection;
	private int dcObjectType = -1;
	
	/**
	 * @param parentShell
	 */
	public SelectNodeDciDialog(Shell parentShell, long nodeId)
	{
		super(parentShell);
		setShellStyle(getShellStyle() | SWT.RESIZE);
		this.nodeId = nodeId;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.SelectNodeDciDialog_Title);
		IDialogSettings settings = Activator.getDefault().getDialogSettings();
		try
		{
			newShell.setSize(settings.getInt("SelectNodeDciDialog.cx"), settings.getInt("SelectNodeDciDialog.cy")); //$NON-NLS-1$ //$NON-NLS-2$
		}
		catch(NumberFormatException e)
		{
			newShell.setSize(400, 250);
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		dialogArea.setLayout(new FillLayout());
		
		dciList = new DciList(null, dialogArea, SWT.BORDER, null, "SelectNodeDciDialog.dciList", dcObjectType);  //$NON-NLS-1$
		dciList.setDcObjectType(dcObjectType);
		dciList.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				okPressed();
			}
		});

		dciList.setNode((Node)((NXCSession)ConsoleSharedData.getSession()).findObjectById(nodeId, Node.class));

		return dialogArea;
	}

	/**
	 * Save dialog settings
	 */
	private void saveSettings()
	{
		Point size = getShell().getSize();
		IDialogSettings settings = Activator.getDefault().getDialogSettings();

		settings.put("SelectNodeDciDialog.cx", size.x); //$NON-NLS-1$
		settings.put("SelectNodeDciDialog.cy", size.y); //$NON-NLS-1$
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#cancelPressed()
	 */
	@Override
	protected void cancelPressed()
	{
		saveSettings();
		super.cancelPressed();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		selection = dciList.getSelection();
		if (selection == null)
		{
			MessageDialog.openWarning(getShell(), Messages.SelectNodeDciDialog_Warning, Messages.SelectNodeDciDialog_WarningText);
			return;
		}
		saveSettings();
		super.okPressed();
	}

	/**
	 * @return the selection
	 */
	public DciValue getSelection()
	{
		return selection;
	}

	/**
	 * @return the dcObjectType
	 */
	public int getDcObjectType()
	{
		return dcObjectType;
	}

	/**
	 * @param dcObjectType the dcObjectType to set
	 */
	public void setDcObjectType(int dcObjectType)
	{
		this.dcObjectType = dcObjectType;
		if (dciList != null)
			dciList.setDcObjectType(dcObjectType);
	}
}
