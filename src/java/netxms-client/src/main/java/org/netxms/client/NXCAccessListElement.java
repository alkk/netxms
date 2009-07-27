package org.netxms.client;

public final class NXCAccessListElement
{
	private long userId;
	private int accessRights;

	public NXCAccessListElement(long userId, int accessRights)
	{
		this.userId = userId;
		this.accessRights = accessRights;
	}

	/**
	 * @return the userId
	 */
	public long getUserId()
	{
		return userId;
	}

	/**
	 * @return the accessRights
	 */
	public int getAccessRights()
	{
		return accessRights;
	}	
	
	@Override
	public boolean equals(Object aThat)
	{
		if (this == aThat)
			return true;
		
		if (!(aThat instanceof NXCAccessListElement))
			return false;
		
		return (this.userId == ((NXCAccessListElement)aThat).userId) &&
		       (this.accessRights == ((NXCAccessListElement)aThat).accessRights);
	}
	
	@Override
	public int hashCode()
	{
		return (int)((accessRights << 16) & userId);
	}
}
