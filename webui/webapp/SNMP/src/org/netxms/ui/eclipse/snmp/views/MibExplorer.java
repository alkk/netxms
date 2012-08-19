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
package org.netxms.ui.eclipse.snmp.views;

import java.util.ArrayList;
import java.util.List;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.TableItem;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IMemento;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Node;
import org.netxms.client.snmp.MibObject;
import org.netxms.client.snmp.SnmpValue;
import org.netxms.client.snmp.SnmpWalkListener;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedColors;
import org.netxms.ui.eclipse.snmp.Activator;
import org.netxms.ui.eclipse.snmp.views.helpers.SnmpValueLabelProvider;
import org.netxms.ui.eclipse.snmp.widgets.MibBrowser;
import org.netxms.ui.eclipse.snmp.widgets.MibObjectDetails;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * SNMP MIB explorer
 *
 */
public class MibExplorer extends ViewPart implements SnmpWalkListener
{
	public static final String ID = "org.netxms.ui.eclipse.snmp.views.MibExplorer";
	
	public static final int COLUMN_NAME = 0;
	public static final int COLUMN_TYPE = 1;
	public static final int COLUMN_VALUE = 2;
	
	private CLabel header;
	private Font headerFont;
	private Color headerColor;
	private MibBrowser mibBrowser;
	private MibObjectDetails details;
	private TableViewer viewer;
	private Node currentNode = null;
	private NXCSession session;
	private boolean walkActive = false;
	private List<SnmpValue> walkData = new ArrayList<SnmpValue>();
	private Action actionRefresh;
	private Action actionWalk;
	private Action actionSetNode;
	private Action actionCopy;
	private Action actionCopyName;
	private Action actionCopyType;
	private Action actionCopyValue;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite, org.eclipse.ui.IMemento)
	 */
	@Override
	public void init(IViewSite site, IMemento memento) throws PartInitException
	{
		if (memento != null)
		{
			long nodeId = safeCast(memento.getInteger("CurrentNode"), 0);
			if (nodeId != 0)
			{
				GenericObject object = ((NXCSession)ConsoleSharedData.getSession()).findObjectById(nodeId);
				if ((object != null) && (object instanceof Node))
					currentNode = (Node)object;
			}
		}
		super.init(site, memento);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		session = (NXCSession)ConsoleSharedData.getSession();
	}

	/**
	 * Safely cast Integer to int - return default value if Integer object is null
	 * 
	 * @param i
	 * @param defval
	 * @return
	 */
	private static int safeCast(Integer i, int defval)
	{
		return (i != null) ? i : defval;
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		headerFont = new Font(parent.getDisplay(), "Verdana", 11, SWT.BOLD);
		headerColor = new Color(parent.getDisplay(), 153, 180, 209);
		
		GridLayout layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.verticalSpacing = 0;
		parent.setLayout(layout);
		
		header = new CLabel(parent, SWT.BORDER);
		header.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
		header.setFont(headerFont);
		header.setBackground(headerColor);
		header.setForeground(SharedColors.WHITE);
		header.setText((currentNode != null) ? currentNode.getObjectName() : "");
		
		SashForm splitter = new SashForm(parent, SWT.VERTICAL);
		splitter.setLayout(new FillLayout());
		splitter.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		
		SashForm mibViewSplitter = new SashForm(splitter, SWT.HORIZONTAL);
		mibViewSplitter.setLayout(new FillLayout());
		
		mibBrowser = new MibBrowser(mibViewSplitter, SWT.BORDER);
		mibBrowser.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				MibObject object = mibBrowser.getSelection();
				details.setObject(object);
			}
		});
		
		details = new MibObjectDetails(mibViewSplitter, SWT.BORDER, true, mibBrowser);
		
		viewer = new TableViewer(splitter, SWT.FULL_SELECTION | SWT.MULTI | SWT.BORDER);
		viewer.getTable().setLinesVisible(true);
		viewer.getTable().setHeaderVisible(true);
		setupViewerColumns();
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new SnmpValueLabelProvider());
		viewer.getTable().addDisposeListener(new DisposeListener() {
			private static final long serialVersionUID = 1L;

			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveColumnSettings(viewer.getTable(), Activator.getDefault().getDialogSettings(), "MibExplorer");
			}
		});

		createActions();
		contributeToActionBars();
		createTreePopupMenu();
		createResultsPopupMenu();
	}
	
	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionRefresh = new RefreshAction() {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				mibBrowser.refreshTree();
			}
		};
		
		actionWalk = new Action("&Walk") {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				doWalk();
			}
		};
		actionWalk.setEnabled(currentNode != null);
		
		actionSetNode = new Action("Set &node object...") {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				ObjectSelectionDialog dlg = new ObjectSelectionDialog(MibExplorer.this.getSite().getShell(), null, ObjectSelectionDialog.createNodeSelectionFilter());
				dlg.enableMultiSelection(false);
				if (dlg.open() == Window.OK)
				{
					setNode((Node)dlg.getSelectedObjects().get(0));
				}
			}
		};
		
		actionCopy = new Action("Copy to clipboard") {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				TableItem[] selection = viewer.getTable().getSelection();
				if (selection.length > 0)
				{
					final String newLine = Platform.getOS().equals(Platform.OS_WIN32) ? "\r\n" : "\n";
					StringBuilder sb = new StringBuilder();
					for(int i = 0; i < selection.length; i++)
					{
						if (i > 0)
							sb.append(newLine);
						sb.append(selection[i].getText(0));
						sb.append(" [");
						sb.append(selection[i].getText(1));
						sb.append("] = ");
						sb.append(selection[i].getText(2));
					}
					WidgetHelper.copyToClipboard(sb.toString());
				}
			}
		};

		actionCopyName = new Action("Copy &name to clipboard") {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				copyColumnToClipboard(0);
			}
		};

		actionCopyType = new Action("Copy &type to clipboard") {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				copyColumnToClipboard(1);
			}
		};

		actionCopyValue = new Action("Copy &value to clipboard") {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				copyColumnToClipboard(2);
			}
		};
	}
	
	/**
	 * Copy values in given column and selected rows to clipboard
	 * 
	 * @param column column index
	 */
	private void copyColumnToClipboard(int column)
	{
		final TableItem[] selection = viewer.getTable().getSelection();
		if (selection.length > 0)
		{
			final String newLine = Platform.getOS().equals(Platform.OS_WIN32) ? "\r\n" : "\n";
			final StringBuilder sb = new StringBuilder();
			for(int i = 0; i < selection.length; i++)
			{
				if (i > 0)
					sb.append(newLine);
				sb.append(selection[i].getText(column));
			}
			WidgetHelper.copyToClipboard(sb.toString());
		}
	}
	
	/**
	 * Fill action bars
	 */
	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	/**
	 * Fill local pull-down menu
	 * @param manager
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionSetNode);
		manager.add(actionWalk);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}
	
	/**
	 * Fill local tool bar
	 * @param manager
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionRefresh);
	}
	
	/**
	 * Create popup menu for MIB tree
	 */
	private void createTreePopupMenu()
	{
		// Create menu manager
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener()
		{
			private static final long serialVersionUID = 1L;

			public void menuAboutToShow(IMenuManager mgr)
			{
				fillTreeContextMenu(mgr);
			}
		});

		// Create menu
		Menu menu = menuMgr.createContextMenu(mibBrowser.getTreeControl());
		mibBrowser.getTreeControl().setMenu(menu);

		// Register menu for extension.
		getSite().registerContextMenu(menuMgr, mibBrowser.getTreeViewer());
	}

	/**
	 * Fill MIB tree context menu
	 * 
	 * @param mgr Menu manager
	 */
	protected void fillTreeContextMenu(final IMenuManager manager)
	{
		manager.add(actionWalk);
		manager.add(new Separator());
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Create popup menu for results table
	 */
	private void createResultsPopupMenu()
	{
		// Create menu manager
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			private static final long serialVersionUID = 1L;

			public void menuAboutToShow(IMenuManager mgr)
			{
				fillResultsContextMenu(mgr);
			}
		});

		// Create menu
		Menu menu = menuMgr.createContextMenu(viewer.getTable());
		viewer.getTable().setMenu(menu);

		// Register menu for extension.
		getSite().registerContextMenu(menuMgr, viewer);
	}

	/**
	 * Fill walk results table context menu
	 * 
	 * @param mgr Menu manager
	 */
	protected void fillResultsContextMenu(final IMenuManager manager)
	{
		manager.add(actionCopy);
		manager.add(actionCopyName);
		manager.add(actionCopyType);
		manager.add(actionCopyValue);
		manager.add(new Separator());
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
	}

	/**
	 * Setup viewer columns
	 */
	private void setupViewerColumns()
	{
		TableColumn tc = new TableColumn(viewer.getTable(), SWT.LEFT);
		tc.setText("OID");
		tc.setWidth(300);

		tc = new TableColumn(viewer.getTable(), SWT.LEFT);
		tc.setText("Type");
		tc.setWidth(100);

		tc = new TableColumn(viewer.getTable(), SWT.LEFT);
		tc.setText("Value");
		tc.setWidth(300);
		
		WidgetHelper.restoreColumnSettings(viewer.getTable(), Activator.getDefault().getDialogSettings(), "MibExplorer");
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		mibBrowser.setFocus();
	}
	
	/**
	 * Set current node
	 * 
	 * @param node
	 */
	public void setNode(Node node)
	{
		currentNode = node;
		actionWalk.setEnabled((node != null) && !walkActive);
		header.setText((currentNode != null) ? currentNode.getObjectName() : "");
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#saveState(org.eclipse.ui.IMemento)
	 */
	@Override
	public void saveState(IMemento memento)
	{
		super.saveState(memento);
		memento.putInteger("CurrentNode", (currentNode != null) ? (int)currentNode.getObjectId() : 0);
	}
	
	/**
	 * Do SNMP walk
	 */
	private void doWalk()
	{
		if (walkActive || (currentNode == null))
			return;
		
		final MibObject object = mibBrowser.getSelection();
		if (object == null)
			return;
		
		walkActive = true;
		actionWalk.setEnabled(false);
		viewer.setInput(new SnmpValue[0]);
		walkData.clear();
		
		ConsoleJob job = new ConsoleJob("Walk MIB tree", this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.snmpWalk(currentNode.getObjectId(), object.getObjectId().toString(), MibExplorer.this);
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot do SNMP MIB tree walk";
			}

			@Override
			protected void jobFinalize()
			{
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						walkActive = false;
						actionWalk.setEnabled(currentNode != null);
					}
				});
			}
		};
		job.setUser(false);
		job.start();
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.snmp.SnmpWalkListener#onSnmpWalkData(java.util.List)
	 */
	@Override
	public void onSnmpWalkData(final List<SnmpValue> data)
	{
		viewer.getControl().getDisplay().asyncExec(new Runnable() {
			@Override
			public void run()
			{
				walkData.addAll(data);
				viewer.setInput(walkData.toArray());
				try
				{
					viewer.getTable().showItem(viewer.getTable().getItem(viewer.getTable().getItemCount() - 1));
				}
				catch(Exception e)
				{
					// silently ignore any possible problem with table scrolling
				}
			}
		});
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		headerFont.dispose();
		headerColor.dispose();
		super.dispose();
	}
}
