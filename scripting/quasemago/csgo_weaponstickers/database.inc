void LoadDatabase()
{
	if (!g_cvarEnabled.BoolValue)
	{
		return;
	}

	if (g_Database != null)
	{
		LogMessage("(WeaponStickers) Database is already connected! (Handle: %d)", g_Database);
		return;
	}

	Database.Connect(SQLCallback_Connect, MySQL_DATABASE);
}

void LoadClientData(int client)
{
	if (g_Database == null)
	{
		return;
	}

	LoadClientStickers(client);
}

void LoadClientStickers(int client)
{
	char authId[MAX_LENGTH_AUTHID];
	if (!GetClientAuthId(client, AuthId_Steam3, authId, sizeof(authId)))
	{
		LogError("Auth failed for client index %d", client);
		return;
	}

	char query[512];
	FormatEx(query, sizeof(query), "SELECT * FROM %s WHERE steamid = \"%s\";", MySQL_TABLE, authId);
	g_Database.Query(SQLCallback_LoadClientStickers, query, GetClientUserId(client));
}

void UpdateClientStickers(int client, int index, int slot)
{
	if (!client || !IsClientInGame(client))
	{
		return;
	}

	char authId[64];
	if (!GetClientAuthId(client, AuthId_Steam3, authId, sizeof(authId)))
	{
		LogError("Auth failed for client index %d", client);
		return;
	}

	// update reuse time.
	g_playerReuseTime[client] = GetTime() + g_cvarReuseTime.IntValue;

	char query[2048];
	FormatEx(query, sizeof(query), "INSERT INTO %s (`steamid`, `index`, `slot%i_index`) VALUES (\"%s\", '%i', '%i') ON DUPLICATE KEY UPDATE slot%i_index = '%i';", MySQL_TABLE, slot, authId, index, g_PlayerWeapon[client][index].m_stickerIndex[slot], slot, g_PlayerWeapon[client][index].m_stickerIndex[slot]);
	g_Database.Query(SQLCallback_UpdateClientStickers, query);
}

/**
 * Callbacks.
 */
public Action Timer_RetryMySQL(Handle timer)
{
	LogMessage("(WeaponStickers) Reconnecting...");

	LoadDatabase();
	return Plugin_Stop;
}

public void SQLCallback_Connect(Database db, const char[] error, any data)
{
	if (db == null || strlen(error) > 0)
	{
		LogError("(WeaponStickers) Connection to database failed: %s", error);
		CreateTimer(5.0, Timer_RetryMySQL);
	}
	else
	{
		DBDriver driver = db.Driver;

		char buffer[32];
		driver.GetIdentifier(buffer, sizeof(buffer));

		if (!StrEqual(buffer, "mysql", false))
		{
			SetFailState("(WeaponStickers) Connection to database failed: Only MySQL support!");
			return;
		}

		delete driver;
		g_Database = db;

		char query[1024];
		FormatEx(query, sizeof(query),
		"CREATE TABLE IF NOT EXISTS `%s` ( \
			`id` INT NOT NULL AUTO_INCREMENT, \
			`steamid` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL, \
			`index` varchar(6) COLLATE utf8mb4_unicode_ci NOT NULL, \
			`slot0_index` int(11) NOT NULL DEFAULT '0', \
			`slot1_index` int(11) NOT NULL DEFAULT '0', \
			`slot2_index` int(11) NOT NULL DEFAULT '0', \
			`slot3_index` int(11) NOT NULL DEFAULT '0', \
			`slot4_index` int(11) NOT NULL DEFAULT '0', \
			`slot5_index` int(11) NOT NULL DEFAULT '0', \
			PRIMARY KEY (`id`), \
			UNIQUE KEY (`steamid`, `index`) \
			) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;", MySQL_TABLE);
		g_Database.Query(SQLCallback_CreateTable, query);

		LogMessage("(WeaponStickers) We have a connection!");
	}
}

public void SQLCallback_CreateTable(Database db, DBResultSet results, const char[] error, any data)
{
	if (db == null || strlen(error) > 0)
	{
		LogError("(SQLCallback_CreateTable) Fail at Query: %s", error);
	}
}

public void SQLCallback_LoadClientStickers(Database db, DBResultSet results, const char[] error, any data)
{
	if (db == null || strlen(error) > 0)
	{
		LogError("(SQLCallback_LoadClientStickers) Fail at Query: %s", error);
	}
	else
	{
		if (results.HasResults)
		{
			int client = GetClientOfUserId(data);

			if (client && IsClientInGame(client))
			{
				while (results.FetchRow())
				{
					// Get weapon index.
					int index = results.FetchInt(2);

					// Check if weapon is valid to stickers.
					if (index != -1 && IsValidWeaponToChange(-1, g_Weapons[index].m_defIndex, _, true))
					{
						g_PlayerWeapon[client][index].m_stickerIndex[0] = results.FetchInt(3);
						g_PlayerWeapon[client][index].m_stickerIndex[1] = results.FetchInt(4);
						g_PlayerWeapon[client][index].m_stickerIndex[2] = results.FetchInt(5);
						g_PlayerWeapon[client][index].m_stickerIndex[3] = results.FetchInt(6);
						g_PlayerWeapon[client][index].m_stickerIndex[4] = results.FetchInt(7);
						g_PlayerWeapon[client][index].m_stickerIndex[5] = results.FetchInt(8);
					}
				}
			}
		}
	}
}

public void SQLCallback_UpdateClientStickers(Database db, DBResultSet results, const char[] error, any data)
{
	if (db == null || strlen(error) > 0)
	{
		LogError("(SQLCallback_UpdateClientStickers) Fail at Query: %s", error);
	}
}