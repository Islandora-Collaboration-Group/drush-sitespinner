<?php
	/**
	 * AUTHORS: Andy Cavenaugh, Pat Dunlavey, David Keiser-Clark
	 *
	 * DESCRIPTION
	 * Sitespinner creates a new Drupal multisite based on an existing live template site.
	 * It uses drush site alias files to describe the source (template) and destination (new) sites.
	 * Sitespinner copies the template site's database and files directory to the new destination site,
	 * and then writes any variables specified in the destination alias to the destination site, overriding the original values.
	 * This allows you to maintain one live, canonical template site (we call ours: "templatesite") from which all new sites will be based.
	 *
	 * This file creates 3 aliases:
	 * 1. "template" - points to the live, pre-existing template site from which settings are fetched for re-use
	 * 2. "parent" - extends "template" alias while offering more granular control for creating children with similar configuration settings
	 * 3. "destination" - you must customize this value! (e.g. "peace")
	 *
	 * COMPLETE INSTRUCTIONS: See https://github.com/Islandora-Collaboration-Group/drush-sitespinner
	 *
	 * CREATE A NEW MULTI-SITE: Specify your "template alias" and your "destination alias"
	 *    drush sitespinner @sitespinner-sample.template @sitespinner-sample.peace
	 *    The command above will create a new multi-site at: https://subdomain.domain.edu/peace
	 *
	 * DELETE A MULTI-SITE: Specify your "destination alias" (e.g "peace")
	 *    drush sitespinner-delete @sitespinner-sample.peace
	 *    The command above will will permanently delete the site's files (sites/{site root} directory), symlink, and corresponding MySQL database and tables
	 *
	 * USEFUL drush commands
	 *    drush sa                               [list all site aliases] ["sa" is shortcut for "site-alias"]
	 *    drush sa @sitespinner-peace            [shows execution view of a specific alias file, without any actual execution]
	 *    drush cc drush                         [clear drush cache] ["cc" is shortcut for "cache-clear"]
	 */

	# ---------------------------
	# Configuration Settings ("Use the Force, Luke.")
	# ---------------------------

	#--- New destination multi-site ---#
	$DESTINATION_SITE_PRETTY_TEXT = "A peaceful new site";      # new site description
	$DESTINATION_SITE_DIRECTORY   = "peace";                    # new site directory name; alphanumeric or dashes; no spaces or underscores; omit trailing slash "/"
	#--- Preexisting template (source) multi-site ---#
	$TEMPLATE_SITE_DIRECTORY = "templatesite";                  # directory of live template site to be copied; alphanumeric or dashes; no spaces or underscores; omit trailing slash "/"
	#--- Server host names ---#
	$DOMAIN_NAME          = "subdomain.domain.edu";             # subdomain.domain.edu; leave empty if none exists; omit "https://"; omit trailing slash "/"
	$FEDORA_HOST_NAME     = "subdomain-fedora.domain.edu";      # full host name (including subdomain, if exists) of server running Fedora (fedora.domain.edu)
	$DRUPAL_WEB_ROOT_PATH = "/var/www/html";                    # "root" is where the web server's index.php file is located, not the multi-site root; omit trailing slash "/"
	$SECURITY_MODE        = "https://";                         # "https://" or "http://"
	#--- MySQL: CREATOR level requires server level permission; SITE level should only have single database permissions ---#
	$MYSQL_HOST_NAME        = "subdomain-webdb.domain.edu";     # full host name (include subdomain, if exists) of server running MySQL (mysql.domain.edu)
	$MYSQL_CREATOR_USER     = "db_creator";                     # "db_creator" - Has permission to create and delete databases
	$MYSQL_CREATOR_PASSWORD = "super_clever_password";          # "super_clever_password"
	$MYSQL_SITE_USER        = "drupal_user";                    # "drupal_user" - Has site specific permissions, but lacks ability to create and delete databases
	$MYSQL_SITE_PASSWORD    = "very_clever_password";           # "very_clever_password"


	# ---------------------------
	# NO NEED TO EDIT ANYTHING BELOW THIS LINE ("These aren't the droids you're looking for.")
	# ---------------------------
	# Sanitize values: convert spaces + underscores to dashes
	$DESTINATION_SITE_DIRECTORY_CLEANED = preg_replace('/[\s+|_]/', '-', $DESTINATION_SITE_DIRECTORY);
	# Dynamically determine the alias groupname (it is the first portion of the filename)
	$alias_groupname = str_replace(".aliases.drushrc.php", "", basename(__FILE__));


	# ---------------------------
	# TEMPLATE alias - This alias provides the mimimum needed to be able to use "@template" as a source site in sitespinner.
	# In a multi-site, the root is the same for all sites.
	# Important: this site must actually exist! (https://subdomain.domain.edu/templatesite)
	# ---------------------------

	$aliases['template'] = array(
		'root'         => $DRUPAL_WEB_ROOT_PATH,
		'uri'          => $DOMAIN_NAME . '/' . $TEMPLATE_SITE_DIRECTORY,
		'db-url'       => 'mysql://' . $MYSQL_SITE_USER . ':' . $MYSQL_SITE_PASSWORD . '@127.0.0.1/drupal_' . $TEMPLATE_SITE_DIRECTORY,
		'path-aliases' => array(
			// REQUIRED: %files path alias on source is required for sitespinner.
			'%files' => $DRUPAL_WEB_ROOT_PATH . '/sites/' . $DOMAIN_NAME . '.' . $TEMPLATE_SITE_DIRECTORY . '/files',
		),
	);


	# ---------------------------
	# PARENT alias - Drush aliases can inherit properties from parent aliases. Here we create an alias called "parent".
	# Important: this template site must actually exist! (https://subdomain.domain.edu/templatesite)
	# This alias is to provide configuration values that multiple "child" sites may inherit and have in common.
	# We don't need to provide everything needed for this to be a functional alias.
	# This alias would not be complete on its own; for example, there is no 'uri' value below.
	# ---------------------------

	$aliases['parent'] = array(
		'root'                    => $DRUPAL_WEB_ROOT_PATH,
		// The "sitespinner-destination" array provides a variety of sitespinner specific information.
		// When inherited from a parent alias, the child "sitespinner-destination" array will be recursively merged to the parent array.
		'sitespinner-destination' => array(
			'db_creator'             => array(
				'username' => $MYSQL_CREATOR_USER,
				'password' => $MYSQL_CREATOR_PASSWORD,
			),
			// Path to the template's settings file. There must be a line "$databases = array();" in that file.
			// This will be replaced by the actual database connection information after the file is copied to the new site's root directory.
			'settings_file_template' => $DRUPAL_WEB_ROOT_PATH . '/sites/' . $DOMAIN_NAME . '.' . $TEMPLATE_SITE_DIRECTORY . '/settings.php',
			// Information about the server environment.
			'server-environment'     => array(
				'default_user'                => getenv('USER'),
				'default_group'               => 'apache',
				'settings_file_permissions'   => '640',
				'files_directory_permissions' => '770',
			),
			// Values that will be pushed into the "variables" db table of the destination site.
			'variables'              => array(
				'site_name'                           => 'Parent Site',
				'islandora_paged_content_djatoka_url' => $SECURITY_MODE . $DOMAIN_NAME . "/adore-djatoka/",
				'islandora_base_url'                  => $SECURITY_MODE . $FEDORA_HOST_NAME . ":8443/fedora",
				'islandora_solr_url'                  => $FEDORA_HOST_NAME . ":8080/solr",
				'imagemagick_convert'                 => "/usr/local/bin/convert",
				'islandora_batch_java'                => "/usr/bin/java",
				'islandora_lame_url'                  => "/usr/local/bin/lame",
				'islandora_paged_content_gs'          => "/usr/bin/gs",
				'islandora_video_ffmpeg_path'         => "/usr/local/bin/ffmpeg",
				'islandora_video_ffmpeg2theora_path'  => "/usr/bin/ffmpeg2theora",
				'islandora_kakadu_url'                => "/usr/local/bin/kdu_compress",
				'islandora_pdf_path_to_pdftotext'     => "/usr/bin/pdftotext",
				'islandora_fits_executable_path'      => "/opt/fits/fits.sh",
				'theme_tao_settings'                  => array(
					"theme_settings"                   => "",
					"toggle_logo"                      => 0,
					"toggle_name"                      => 0,
					"toggle_slogan"                    => 0,
					"toggle_node_user_picture"         => 0,
					"toggle_comment_user_picture"      => 0,
					"toggle_comment_user_verification" => 0,
					"toggle_favicon"                   => 0,
					"toggle_main_menu"                 => 0,
					"toggle_secondary_menu"            => 0,
					"logo"                             => "",
					"default_logo"                     => 0,
					"logo_path"                        => "",
					"logo_upload"                      => "",
					"default_favicon"                  => 0,
					"favicon_path"                     => "public://home-logo_4.jpg",
					"favicon_upload"                   => "",
					"project_header_image_fid"         => "",
					"background_image_folder"          => "gray-glow",
					"mission_statement"                => array(
						"value"  => '',
						"format" => "filtered_html",
					),
					"favicon_mimetype"                 => "image/jpeg",
				),
			),
		),
	);


	# ---------------------------
	# DESTINATION alias - This alias sets the name of your new (destination) multi-site.
	# Must provide all the mandatory site alias information either here, or via inheritance from a parent.
	# ---------------------------

	$aliases[$DESTINATION_SITE_DIRECTORY] = array(
		// Inherit configuration from the 'parent' alias.
		'parent'                  => '@' . $alias_groupname . '.parent',
		'root'                    => $DRUPAL_WEB_ROOT_PATH,
		'uri'                     => $DOMAIN_NAME . '/' . $DESTINATION_SITE_DIRECTORY,
		'path-aliases'            => array(
			'%dump'  => '/home/islandora/db_dump_' . $DESTINATION_SITE_DIRECTORY . '.sql',
			// REQUIRED: %files path alias on destination is required for sitespinner.
			'%files' => $DRUPAL_WEB_ROOT_PATH . '/sites/' . $DOMAIN_NAME . '.' . $DESTINATION_SITE_DIRECTORY . '/files',
		),
		// A complete database definition array is required for sitespinner.
		// This will be written as-is to the $databases variable in the settings.php file, which will be created (if it doesn't already exist).
		'databases'               => array(
			'default' => array(
				'default' => array(
					'database' => 'drupal_' . $DESTINATION_SITE_DIRECTORY,
					'username' => $MYSQL_SITE_USER,
					'password' => $MYSQL_SITE_PASSWORD,
					'host'     => '127.0.0.1',
					'port'     => '3306',
					'driver'   => 'mysql',
					'prefix'   =>
						array(
							'default'                 => '',
							'ldap_authorization'      => 'drupal_prime.',
							'ldap_servers'            => 'drupal_prime.',
							'taxonomy_index'          => 'drupal_prime.',
							'taxonomy_term_data'      => 'drupal_prime.',
							'taxonomy_term_hierarchy' => 'drupal_prime.',
							'taxonomy_vocabulary'     => 'drupal_prime.',
						),
				),
			),
		),
		'sitespinner-destination' => array(
			// Define the way the site will be instantiated as a multi-site, 'type' may be be one of either 'path', 'domain', or 'subdomain':
			// 'path' means that the site will be accessed using a path alias under the default site's domain (e.g. domain.edu/path).
			// 'domain' means that the site will have it's own domain name (e.g. domain.edu).
			// 'subdomain' means that the site will be created as a subdomain of the default site's domain (e.g. subdomain.domain.edu).
			// 'name' defines the name to use for the given domain type.
			'create-domain' => array(
				'type' => 'path',
				'name' => $DESTINATION_SITE_DIRECTORY,
			),
			// THEME SETTINGS are automatically fetched from your live template site.
			// (You may provide the entire nested array of theme settings below to override your template site.)
			// (You cannot merge overrided theme setting values with fetched values.)
			// All theme values are pushed into the mysql 'variables' table.
			'variables'     => array(
				'site_name' => $DESTINATION_SITE_PRETTY_TEXT,
			),
		),
	);
