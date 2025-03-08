// ВСЕ SQL запросы
namespace SQL
{
    const char get_aval_id[] = R"(
WITH available_ids AS (
    SELECT generate_series(0, (SELECT COALESCE(MAX(id) + 1, 0) FROM watchers)) AS id
)
SELECT id FROM available_ids
WHERE id NOT IN (SELECT id FROM watchers)
ORDER BY id
LIMIT 1;   
)";
    const char verify_exists_1s_tables_unit[]=R"(
SELECT EXISTS (SELECT 1 FROM watchers); 
)";
    const char get_all_watchers[]=R"(
    SELECT * FROM watchers ORDER BY id ASC;
)";
    const char get_all_dates[]=R"(
SELECT * FROM dates;
    )";
    const char get_all_users[]=R"(
SELECT id FROM users WHERE isAutosend=TRUE;
)";
};