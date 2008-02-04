use 5.8.3;
use ExtUtils::MakeMaker;

WriteMakefile(
    NAME              => 'SWISH::3',
    VERSION_FROM      => 'lib/SWISH/3.pm',
    PREREQ_PM         => {}, 
    ($] >= 5.005 ?     
      (ABSTRACT_FROM  => 'lib/SWISH/3.pm',
       AUTHOR         => 'Peter Karman <perl@peknet.com>') : ()),
    LIBS              => ['-L/usr/lib -L/usr/local/lib -lswish3 -lxml2'],
    DEFINE            => '', # e.g., '-DHAVE_SOMETHING'
    INC               => '-I/usr/include -I/usr/local/include -I/usr/include/libxml2 -I/usr/local/include/libxml2 -I.',
	# Un-comment this if you add C files to link with later:
    # OBJECT            => '$(O_FILES)', # link all the C files too
);
