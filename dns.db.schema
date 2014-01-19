PRAGMA foreign_keys = ON;
PRAGMA auto_vacuum = FULL;
PRAGMA synchronous = OFF;
PRAGMA locking_mode = EXCLUSIVE;


BEGIN TRANSACTION;

CREATE TABLE domains (
  domain_id INTEGER PRIMARY KEY,
  domain_name VARCHAR(255) NOT NULL COLLATE NOCASE,
  master VARCHAR(128) DEFAULT NULL COLLATE NOCASE,
  email VARCHAR(128) DEFAULT NULL COLLATE NOCASE,
  serial INTEGER DEFAULT NULL,
  refresh INTEGER DEFAULT 86400,
  retry INTEGER DEFAULT 7200,
  expiration INTEGER DEFAULT 2419200,
  cache INTEGER DEFAULT 3600
);

CREATE UNIQUE INDEX name_index ON domains(domain_name);

CREATE TABLE types (
  type_id INTEGER PRIMARY KEY,
  type_name VARCHAR(10) NOT NULL UNIQUE
);

CREATE TABLE records (
  record_id INTEGER PRIMARY KEY,
  domain_id INTEGER NOT NULL REFERENCES domains ON DELETE CASCADE ON UPDATE CASCADE,
  record_name VARCHAR(255) DEFAULT NULL COLLATE NOCASE,
  type_id INTEGER NOT NULL REFERENCES types ON DELETE RESTRICT ON UPDATE CASCADE,
  content VARCHAR(65535) NOT NULL,
  ttl INTEGER DEFAULT 3600,
  prio INTEGER DEFAULT NULL,
  change_date DATE DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX record_name_index ON records(record_name);
CREATE UNIQUE INDEX record_index ON records(record_name, domain_id, type_id);
CREATE INDEX domain_id ON records(domain_id);

CREATE TABLE ddns_clients (
  ddns_client_id INTEGER PRIMARY KEY,
  record_id INTEGER NOT NULL REFERENCES records ON DELETE CASCADE ON UPDATE CASCADE,
  id_string VARCHAR(64) NOT NULL UNIQUE
);

CREATE TABLE content_logs (
  log_id INTEGER PRIMARY KEY,
  record_id INTEGER REFERENCES records ON DELETE CASCADE ON UPDATE CASCADE,
  content_old VARCHAR(65535),
  content_new VARCHAR(65535),
  change_date DATE DEFAULT CURRENT_TIMESTAMP
);

CREATE VIEW records_j AS
  SELECT r.record_id AS id, r.record_name AS name, d.domain_name AS domain,
    t.type_name AS type, r.content, r.ttl, r.prio, r.change_date
  FROM records r 
  JOIN domains d USING(domain_id)
  JOIN types t USING(type_id);

CREATE TRIGGER upper_types AFTER INSERT ON types
BEGIN
  UPDATE types
  SET type_name = upper(NEW.type_name)
  WHERE type_id = NEW.type_id;
END;

CREATE TRIGGER upper_types_update AFTER UPDATE OF type_name ON types
BEGIN
  UPDATE types
  SET type_name = upper(NEW.type_name)
  WHERE type_id = NEW.type_id;
END;

CREATE TRIGGER content_log AFTER UPDATE OF content ON records
BEGIN
  INSERT INTO content_logs (record_id, content_old, content_new)
  VALUES (OLD.record_id, OLD.content, NEW.content);
END;

CREATE TRIGGER insert_domain AFTER INSERT ON domains
BEGIN
  UPDATE domains
  SET domain_name = domain_name || '.'
  WHERE domain_id = NEW.domain_id AND domain_name NOT LIKE '%.';
  UPDATE domains
  SET master = 'ns1.' || domain_name
  WHERE domain_id = NEW.domain_id AND NEW.master IS NULL;
  UPDATE domains
  SET email = 'postmaster.' || domain_name
  WHERE domain_id = NEW.domain_id AND NEW.email IS NULL;
  UPDATE domains
  SET serial = (strftime('%Y%m%d','NOW') * 100) 
  WHERE domain_id = NEW.domain_id;
END;

CREATE TRIGGER serial_increment_domains AFTER UPDATE ON domains
WHEN OLD.domain_name <> NEW.domain_name OR OLD.master <> NEW.master
BEGIN
  UPDATE domains
  SET serial = (strftime('%Y%m%d','NOW') * 100) 
  WHERE domain_id = NEW.domain_id 
  AND (strftime('%Y%m%d','NOW') * 100) > (
    SELECT serial
    FROM domains
    WHERE domain_id = NEW.domain_id
  );
  UPDATE domains 
  SET serial = ((
    SELECT serial 
    FROM domains
    WHERE domain_id = NEW.domain_id
  ) + 1) 
  WHERE domain_id = NEW.domain_id;
END;

CREATE TRIGGER serial_increment_records AFTER INSERT ON records
BEGIN
  UPDATE domains
  SET serial = (strftime('%Y%m%d','NOW') * 100) 
  WHERE domain_id = NEW.domain_id 
  AND (strftime('%Y%m%d','NOW') * 100) > (
    SELECT serial
    FROM domains
    WHERE domain_id = NEW.domain_id
  );
  UPDATE domains 
  SET serial = ((
    SELECT serial 
    FROM domains
    WHERE domain_id = NEW.domain_id
  ) + 1) 
  WHERE domain_id = NEW.domain_id;
END;

CREATE TRIGGER update_serial_and_date AFTER UPDATE ON records
BEGIN
  UPDATE domains
  SET serial = (strftime('%Y%m%d','NOW') * 100) 
  WHERE domain_id = NEW.domain_id 
  AND (strftime('%Y%m%d','NOW') * 100) > (
    SELECT serial
    FROM domains
    WHERE domain_id = NEW.domain_id
  );
  UPDATE domains 
  SET serial = ((
    SELECT serial 
    FROM domains
    WHERE domain_id = NEW.domain_id
  ) + 1) 
  WHERE domain_id = NEW.domain_id;
  UPDATE records
  SET change_date = DATETIME('NOW')
  WHERE record_id = NEW.record_id;
END;

CREATE TRIGGER insert_record INSTEAD OF INSERT ON records_j
BEGIN
  INSERT INTO records (domain_id, record_name, type_id, content, ttl, prio)
  VALUES(
    (SELECT domain_id FROM domains WHERE domain_name IN (NEW.domain, NEW.domain || '.')),
    NEW.name,
    (SELECT type_id FROM types WHERE type_name = upper(NEW.type)),
    NEW.content,
    NEW.ttl,
    NEW.prio
  );
END;

CREATE TRIGGER update_content INSTEAD OF UPDATE OF content ON records_j
BEGIN
  UPDATE records
  SET content = NEW.content
  WHERE record_name = NEW.name AND type_id = (
    SELECT type_id 
    FROM types 
    WHERE type_name = upper(NEW.type)
  );
END;

CREATE TRIGGER update_ttl INSTEAD OF UPDATE OF ttl ON records_j
BEGIN
  UPDATE records
  SET ttl = NEW.ttl
  WHERE record_name = NEW.name AND type_id = (
    SELECT type_id
    FROM types
    WHERE type_name = upper(NEW.type)
  );
END;

CREATE TRIGGER update_prio INSTEAD OF UPDATE OF prio ON records_j
BEGIN
  UPDATE records
  SET prio = NEW.prio
  WHERE record_name = NEW.name AND type_id = (
    SELECT type_id
    FROM types
    WHERE type_name = upper(NEW.type)
  );
END;

CREATE TRIGGER delete_content INSTEAD OF DELETE ON records_j
BEGIN
  DELETE FROM records
  WHERE record_name = NEW.name AND type_id = (
    SELECT type_id
    FROM types
    WHERE type_name = upper(NEW.type));
END;

INSERT INTO types(type_name) VALUES('NS'),('A'),('AAAA'),('CNAME'),('MX'),('TXT'),('SRV');

COMMIT;
