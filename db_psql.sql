BEGIN WORK;

CREATE TABLE domains (
  domain_id SERIAL PRIMARY KEY,
  domain_name VARCHAR(255) NOT NULL,
  master VARCHAR(128) DEFAULT NULL,
  email VARCHAR(128) DEFAULT NULL,
  domain_serial INTEGER DEFAULT NULL,
  refresh INTEGER DEFAULT 86400,
  retry INTEGER DEFAULT 7200,
  expiration INTEGER DEFAULT 2419200,
  cache INTEGER DEFAULT 3600,
  CONSTRAINT c_lowercase_domain_name CHECK (((domain_name)::text = lower((domain_name)::text))),
  CONSTRAINT c_lowercase_master CHECK (((master)::text = lower((master)::text))),
  CONSTRAINT c_lowercase_email CHECK (((email)::text = lower((email)::text)))
);

CREATE UNIQUE INDEX name_index ON domains(domain_name);

CREATE TABLE types (
  type_id SERIAL PRIMARY KEY,
  type_name VARCHAR(10) NOT NULL UNIQUE
  CONSTRAINT c_uppercase_type_name CHECK (((type_name)::text = upper((type_name)::text)))
);

CREATE TABLE records (
  record_id SERIAL PRIMARY KEY,
  domain_id INTEGER NOT NULL REFERENCES domains ON DELETE CASCADE ON UPDATE CASCADE,
  record_name VARCHAR(255) DEFAULT NULL,
  type_id INTEGER NOT NULL REFERENCES types ON DELETE RESTRICT ON UPDATE CASCADE,
  content VARCHAR(65535) NOT NULL,
  ttl INTEGER DEFAULT 3600,
  prio INTEGER DEFAULT NULL,
  change_date TIMESTAMP DEFAULT now(),
  CONSTRAINT c_lowercase_record_name CHECK (((record_name)::text = lower((record_name)::text)))
);

CREATE UNIQUE INDEX record_index ON records(record_name, domain_id, type_id);
CREATE INDEX domain_id ON records(domain_id);

CREATE TABLE ddns_clients (
  ddns_client_id SERIAL PRIMARY KEY,
  record_id INTEGER NOT NULL REFERENCES records ON DELETE CASCADE ON UPDATE CASCADE,
  id_string VARCHAR(64) NOT NULL
);

CREATE UNIQUE INDEX id_string_index ON ddns_clients(id_string);

CREATE TABLE content_logs (
  log_id SERIAL PRIMARY KEY,
  record_id INTEGER REFERENCES records ON DELETE CASCADE ON UPDATE CASCADE,
  content_old VARCHAR(65535),
  content_new VARCHAR(65535),
  change_date TIMESTAMP DEFAULT now()
);

CREATE VIEW records_j AS
  SELECT r.record_id AS id, r.record_name AS name, d.domain_name AS domain,
    t.type_name AS type, r.content, r.ttl, r.prio, r.change_date
  FROM records r 
  JOIN domains d USING(domain_id)
  JOIN types t USING(type_id);

CREATE OR REPLACE FUNCTION log_content_change() RETURNS trigger AS $$
BEGIN
  INSERT INTO content_logs (record_id, content_old, content_new)
  VALUES (OLD.record_id, OLD.content, NEW.content);
  RETURN NEW;
END;
$$ LANGUAGE 'plpgsql';

CREATE TRIGGER content_log AFTER UPDATE OF content ON records
FOR EACH ROW EXECUTE PROCEDURE log_content_change();

CREATE OR REPLACE FUNCTION sanitize_domain_input() RETURNS trigger AS $$
BEGIN
  IF NEW.domain_name NOT LIKE '%.' THEN
  NEW.domain_name = NEW.domain_name || '.';
  END IF;
  IF NEW.master IS NULL THEN
  NEW.master = 'ns1.' || NEW.domain_name;
  END IF;
  IF NEW.email IS NULL THEN
  NEW.email = 'postmaster.' || NEW.domain_name;
  END IF;
  NEW.domain_serial = to_char(now(), 'YYYYMMDD01')::integer;
  RETURN NEW;
END;
$$ LANGUAGE 'plpgsql';

CREATE TRIGGER sanitize_input BEFORE INSERT ON domains
FOR EACH ROW EXECUTE PROCEDURE sanitize_domain_input();

CREATE OR REPLACE FUNCTION increment_serial() RETURNS trigger AS $$
DECLARE
  old_serial INTEGER := (SELECT domain_serial FROM domains WHERE domain_id = NEW.domain_id);
  now_serial INTEGER := to_char(now(), 'YYYYMMDD01')::integer;
  new_serial INTEGER;
BEGIN
  IF now_serial > old_serial THEN
    new_serial := now_serial;
  ELSE
    new_serial := old_serial + 1;
  END IF;
  UPDATE domains
  SET domain_serial = new_serial
  WHERE domain_id = NEW.domain_id ;
  RETURN NEW;
END;
$$ LANGUAGE 'plpgsql';

CREATE TRIGGER domain_serial_increment AFTER UPDATE ON domains
FOR EACH ROW 
WHEN (OLD.domain_name <> NEW.domain_name OR OLD.master <> NEW.master)
EXECUTE PROCEDURE increment_serial();

CREATE TRIGGER domain_serial_increment_records AFTER INSERT OR UPDATE OR DELETE ON records
FOR EACH ROW EXECUTE PROCEDURE increment_serial();

CREATE OR REPLACE FUNCTION update_date() RETURNS trigger AS $$
BEGIN
  NEW.change_date = now();
  RETURN NEW;
END;
$$ LANGUAGE 'plpgsql';

CREATE TRIGGER update_record_date BEFORE UPDATE ON records
FOR EACH ROW EXECUTE PROCEDURE update_date();

CREATE OR REPLACE FUNCTION insert_records_j() RETURNS trigger AS $$ 
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
  RETURN NEW;
END;
$$ LANGUAGE 'plpgsql';

CREATE TRIGGER insert_record INSTEAD OF INSERT ON records_j
FOR EACH ROW EXECUTE PROCEDURE insert_records_j();

--CREATE OR REPLACE FUNCTION update_record() RETURNS trigger AS $$
--BEGIN
--  NEW.type_id = ( 
--        SELECT type_id
--        FROM types
--        WHERE type_name = upper(NEW.type));
--END;
--$$ LANGUAGE 'plpgsql';

--CREATE TRIGGER update_content BEFORE UPDATE OR DELETE ON records_j
----FOR EACH ROW EXECUTE PROCEDURE update_record();

INSERT INTO types(type_name) VALUES('NS'),('A'),('AAAA'),('CNAME'),('MX'),('TXT'),('SRV');

COMMIT WORK;
