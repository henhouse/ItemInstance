// This tool converts the item_instance table from mangoszero to vmangos format.
// Author: brotalnia
// Adapted for use in cMaNGOS by Henhouse

#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <array>
#include <set>
#include <sstream>

#include "Database\Database.h"

Database GameDb;

std::string MakeConnectionString()
{
    std::string mysql_host;
    std::string mysql_port;
    std::string mysql_user;
    std::string mysql_pass;
    std::string mysql_db;

    printf("Host: ");
    getline(std::cin, mysql_host);
    if (mysql_host.empty())
        mysql_host = "127.0.0.1";

    printf("Port: ");
    getline(std::cin, mysql_port);
    if (mysql_port.empty())
        mysql_port = "3306";

    printf("User: ");
    getline(std::cin, mysql_user);
    if (mysql_user.empty())
        mysql_user = "root";

    printf("Password: ");
    getline(std::cin, mysql_pass);
    if (mysql_pass.empty())
        mysql_pass = "root";

    printf("Database: ");
    getline(std::cin, mysql_db);
    if (mysql_db.empty())
        mysql_db = "character";

    return mysql_host + ";" + mysql_port + ";" + mysql_user + ";" + mysql_pass + ";" + mysql_db;
}

void ParseDataString(std::vector<uint32>& values, std::string const& data)
{
    std::string temp_string;

    for (char const symbol : data)
    {
        if (isdigit(symbol))
            temp_string += symbol;
        else if (!temp_string.empty())
        {
            values.push_back(strtoul(temp_string.c_str(), NULL, 0));
            temp_string = "";
        }
    }
}

std::string GetSaveString(std::vector<uint32>& values, int expansion)
{
    uint32 item_guid = values[0];
    //uint32 unknown = values[1];
    //uint32 type = values[2];
    uint32 entry = values[3];
    //float scale_x = *((float*)&values[4]);
    //uint32 padding = values[5];
    uint64 owner_guid = *((uint64*)&values[6]);
    uint64 contained = *((uint64*)&values[8]);
    uint64 creator_guid = *((uint64*)&values[10]);
    uint64 gift_creator = *((uint64*)&values[12]);
    uint32 stack_count = values[14];
    uint32 duration = values[15];

    std::string spell_charges;
    for (int i = 16; i < 21; i++)
    {
        if (i != 16)
            spell_charges += " ";

        spell_charges += std::to_string(*((int*)&values[i]));
    }

    uint32 flags = values[21];

    std::string enchantments;
    for (int i = 22; i < 55; i++)
    {
        if (i != 22)
            enchantments += " ";

        enchantments += std::to_string(values[i]);
    }

    //uint32 property_seed = values[55];
    int32 random_property_id = *((int32*)&values[56]); // Stored as uint32, but we want int32 representation.
    uint32 text_id = values[57];
    uint32 durability = values[58];
    //uint32 max_durability = values[59];

    std::ostringstream return_string;
    return_string << "(" << item_guid << ", " << owner_guid << ", " << entry << ", " << creator_guid << ", " << gift_creator << ", " << stack_count << ", " << duration << ", '" << spell_charges << "', " << flags << ", '" << enchantments << "', " << random_property_id << ", " << durability << ", " << text_id << ")";
    return return_string.str();
}

int main()
{
    printf("\n\n####################################################\n");
    printf("# BACKUP YOUR DATABASE BEFORE RUNNING THIS SCRIPT! #\n");
    printf("####################################################\n");

    printf("\nSpecify expansion data to migrate: (Type 1-3 and press Enter)\n");
    printf("\t1. Vanilla\n");
    printf("\t2. TBC\n");
    printf("\t3. WotLK\n");
    printf("> ");

    int expansion = 0;
    if (!(std::cin >> expansion))
    {
        printf("\nBad input.\n");
        return 1;
    }

    switch (expansion)
    {
    case 1:
    case 2:
    case 3:
        break;
    default:
        printf("\nInvalid expansion specified.\n");
        return 1;
    }

    printf("\nEnter your database connection info.\n");
    std::string const connection_string = MakeConnectionString();

    printf("\nConnecting to database.\n");
    if (!GameDb.Initialize(connection_string.c_str()))
    {
        printf("\nError: Cannot connect to character database!\n");
        getchar();
        return 1;
    }

    uint32 items_count = 0;

    printf("Loading item database.\n");
    //                                                              0       1             2      
    if (std::shared_ptr<QueryResult> result = GameDb.Query("SELECT `guid`, `owner_guid`, `data` FROM `item_instance`"))
    {
        std::ofstream myfile("item_instance.sql");
        if (!myfile.is_open())
            return 1;

        myfile << "TRUNCATE `item_instance`;\n";
        myfile << "ALTER TABLE `item_instance` DROP `data`;\n\n";

        myfile << "ALTER TABLE `item_instance`\n";
        myfile << " ADD `itemEntry` MEDIUMINT(8) UNSIGNED NOT NULL DEFAULT '0' AFTER `owner_guid`,\n";
        myfile << " ADD `creatorGuid` INT(10) UNSIGNED NOT NULL DEFAULT '0' AFTER `itemEntry`,\n";
        myfile << " ADD `giftCreatorGuid` INT(10) UNSIGNED NOT NULL DEFAULT '0' AFTER `creatorGuid`,\n",
        myfile << " ADD `count` INT(10) UNSIGNED NOT NULL DEFAULT '1' AFTER `giftCreatorGuid`,\n";
        myfile << " ADD `duration` INT(10) UNSIGNED NOT NULL AFTER `count`,\n";
        myfile << " ADD `charges` TEXT NOT NULL AFTER `duration`,\n";
        myfile << " ADD `flags` INT(10) UNSIGNED NOT NULL DEFAULT '0' AFTER `charges`,\n";
        myfile << " ADD `enchantments` TEXT NOT NULL AFTER `flags`,\n";
        myfile << " ADD `randomPropertyId` INT(11) NOT NULL DEFAULT '0' AFTER `enchantments`,\n";
        myfile << " ADD `durability` INT(10) UNSIGNED NOT NULL DEFAULT '0' AFTER `randomPropertyId`,\n";

        if (expansion != 3)
            myfile << " ADD `itemTextId` MEDIUMINT(8) UNSIGNED NOT NULL DEFAULT '0' AFTER `durability`;\n\n";
        else
            myfile << " ADD `playedTime` int(10) unsigned NOT NULL default '0' AFTER `durability`;\n\n";

        myfile << "INSERT INTO `item_instance` ";

        switch (expansion)
        {
        case 1: // Vanilla
        case 2: // TBC
            myfile << "(guid, owner_guid, itemEntry, creatorGuid, giftCreatorGuid, count, duration, charges, flags, enchantments, randomPropertyId, durability, ";
            myfile << ((expansion != 3) ? "itemTextId" : "playedTime");
            myfile << ")";
            break;
        case 3: // WotLK
            break;
        }
        myfile << " VALUES \n";

        do
        {
            items_count++;
            DbField* pFields = result->fetchCurrentRow();

            uint32 item_guid = pFields[0].getUInt32();
            uint32 owner_guid = pFields[1].getUInt32();
            std::string data = pFields[2].getCppString();

            if (items_count > 1)
                myfile << ",\n";

            std::vector<uint32> values;
            ParseDataString(values, data);
            myfile << GetSaveString(values, expansion);

        } while (result->NextRow());

        myfile << ";\n";
        myfile.close();
    }

    printf("Done. Converted %u item entries.", items_count);
    getchar();
    
    GameDb.Uninitialise();
    return 0;
}
