#### Download And Install

You can find the latest version on bitbucket:
[bitbucket.org/commonmedia/drush-sitespinner](https://bitbucket.org/commonmedia/drush-sitespinner)
Best option is to cd into your drush/commands folder, and clone the script:

     git clone git@bitbucket.org:commonmedia/drush-sitespinner.git

#### Introduction

Sitespinner provides two drush commands: sitespinner (ss), and 
sitespinner-delete (ssd).

**Sitespinner** creates a new Drupal multisite based on an existing template site. 
It uses drush site alias files to describe both the source and destination 
sites. The source site's database and files directory are copied to the 
destination site, and then any variables specified in the destination alias are
written on the destination site, overriding the original values.

This is a novel approach to using site aliases, since normally they are used to
describe sites that already exist. It occurred to us as we began to think about
how to supply configuration options to a drush command, that extending the idea 
of site alias files made a lot of sense. A standard site alias file already 
provides information about where the site lives and how it is eached ('root' 
and 'uri'), how it connects to the database, where it's files directory is, etc.

Once source and destination alias files are created, then the command is simply:

    drush sitespinner @source-alias @destination-alias

**Sitespinner-delete** simply deletes a multisite, given the site alias for that
site. Specifically, it deletes associated database, removes the files directory,
removes the sites/{site root} directory, and deletes the multisite symlink at
the drupal root.
    
#### Requirements

* [drush-All-Versions-2.0.tar.gz](http://ftp.drupal.org/files/projects/drush-All-Versions-2.0.tar.gz) Or later.
* Or any later version should work.
* PHP, Apache2, MySQL or PostgreSQL
* sudo (and root privileges) (on other Debian you may need to do:

    su root

* Access to creating databases (MySQL or PostgreSQL or both)

#### Usage

To use this command, you must have a working Drupal site that either already
is functioning as a multisite, or that you want to function as a multisite. You must
have created a site alias file for the existing (source) site that you wish to
use as a template for the new multisite. This can be the root (default) site,
or another multisite found in the sites directory. Finally, you will need to
create a site alias file for the new (destination) multisite that you wish to
create.
 
Consult the sample aliases file found within the examples folder. Also, consult
the documentation for drush alias files:

    drush topic docs-aliases

Alias files are typically installed at ~/.drush/. Consult the above documentation
for alternate locations.

To be used as a source alias with sitespinner, the source alias file, at a
minimum, needs to provide values for these:

    ['root'] (The Drupal root)
    ['uri']
    ['db_url'] or ['databases']
    ['path-aliases']['%files']

To be used as a destination site alias, the file needs to provide, at a minimum,
values for these:

    ['root'] (This is the drupal root, not the path to the actual multisite's directory under /sites/.)
    ['uri']
    ['databases']['default']['default'] (IMPORTANT: ['db_url'] will not work!!!!)
    ['path-aliases']['%files']
    
Other sitespinner-specific values that you should normally provide:

    ['sitespinner-destination']['db_creator'] = array(
        'username' => "...",
        'password' => "...",
    );
        Since you will be creating a database, you will probably need 
        to provide a mysql username and password for a more privileged 
        user than your standard drupal database user. If not provided, then
        this command will attempt to create the database using the default
        drupal database connection info.
        
    ['sitespinner-destination']['settings_file_template'] 
        A path to a file to use as a settings.php template. It must contain a
        line with the text  "$databases = array();". If not provided, the 
        /sites/default/default.settings.php file will be used.
        
    ['sitespinner-destination']['server-environment'] = array(
        'default_user' => "...",
        'default_group' => "...",
        'settings_file_permissions' => '640',
        'files_directory_permissions' => '770',
    );
        Users, groups, and permissions for the files and directories that will be created.
        
    ['sitespinner-destination']['create-domain'] = array(
        'type' => 'path',
        'name' => '...',
    );
        TODO: Not fully implemented. Read the notes in the sample aliases file.

Variable overrides. Any key => value pairs that you put into the variables array
will be written to the variables table in the destination site's database. In this way
you can specify the site name, theme settings, and many other configuration options.
Any array values will be recursively merged with values provided in inherited (parent) alias
files.

    ['sitespinner-destination']['variables'] = array(
        'key' => 'value',
        'another key' => array(
            'cool' => 'I can write arrays to the variables table!'
        ),
    );

#### Using parent aliases

A little known feature of drush site aliases is that you can specify a comma-separated
list of site aliases whose properties will be inherited by this alias. To do so, add a
'parent' key to the alias array:

    'parent' => '@etc, @grandparent, @parent',

This can be handy when managing multiple configurations of sites in your multisite
setup. You can define the characteristics that are common to a given configuration
in the parent alias, and then in the child site, just provide those details that
are needed for that site.

#### Options

You can use the following flags:

    --simulate

flag to see what the commands will do to your system. When using simulate there
will be no changes to your system.

    -f

Force the script to continue when errors are encountered. E.g.:
You have tried to install a site but your failed halfway due to permissions.
You can then change the permissions on your server, and force the script to execute.

    -s

Silence the script will produce less output, and only prompt you when
questions are raised.