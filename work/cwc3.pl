#!/usr/bin/perl -w
#Depends on the program maxtime. I could make my own but this was quicker.

#tcpdump -i eth1 -lx -s 1024 -w dota_publicgame.log tcp port 6112

#WC3 protocol format:
#[2 byte type number] [size of the data (starting from the type number)] 00 [rest of packet data]

#Notes:
#- I'm not sure if the 00 after the size of the data is part of the size also. But, so far I haven't
#  found data large enough (over 255 bytes) to help me decide that.
#- It seems to me that any found offset, is relative to where the start of the 2 byte type number is.
#- Type numbers prefixed with FF is data sent to/received from battle.net. (1)
#- Type numbers prefixed with F7 is data sent to/received from clients.    (1)
#- WC3 loves to combine packets. But this isn't a problem. We know that the first 2 bytes is the type number,
#  and the 3rd byte is the size. So, if we find that the size of the whole tcppacket->{data} is bigger than
#  the size reported by the 3rd byte, then we know we have more data to process.

#Point (1):
#Since this is true, you probably don't have to divide the packet processing in 4 sections in ProcessPacket()
#but only in two of them. Whether you're dealing with battle.net or clients. But it really doesn't matter.
#If you find more prefixes which are acceptable, be sure to add it to @g_data_prefixes.

#-----------------------------------------#
# Includes
#-----------------------------------------#

use Net::Pcap;           #Packet capture
use NetPacket::Ethernet; #Used to easily rip out the IP/TCP packet
use NetPacket::IP;       #Used to easily get the info that I need from an IP packet
use NetPacket::TCP;      #Used to easily get the info that I need from an TCP packet
use Class::Struct;       #This is enabled because of my C++ coding ethics
use Curses;              #Pretty interface
use Cdk;                 #Helps make the pretty interface
use threads;             #So we can listen to sockets and update ncurses
use threads::shared;     #Some variables need to be shared
use Geo::IP;             #Country code lookups
use strict;              #This is enabled because of my C++ coding ethics

#-----------------------------------------#
# Settings
#-----------------------------------------#

my $g_device            = "eth1";
my $g_gamehost          = "192.168.0.2";
my $g_gameport          = 6112;
my $g_battleport        = 6112;
my @g_allowed_countries = ("US", "CA");

#-----------------------------------------#
# Client data
#-----------------------------------------#

#The primary key is the ip + port.
struct Client => {
  name      => '$',
  id		    => '$',
  color     => '$',
  slot      => '$',
  team      => '$',
  teamname  => '$',
  ip        => '$',
  port      => '$',
  flags     => '$',
  connected => '$',
  country   => '$',
};

#-----------------------------------------#
# Events
#-----------------------------------------#

struct Event => {
  creatinggame  => '$', #boolean
  createdgame   => '$', #boolean
  startinggame  => '$', #boolean
  startedgame   => '$', #boolean
  canceledgame  => '$', #boolean
  endedgame     => '$', #boolean
};

#-----------------------------------------#
# Ncurses objects
#-----------------------------------------#

my $g_coutput;
my $g_clist;
my $g_cinput;
my $g_cinfo;

#-----------------------------------------#
# Settings the user shouldn't touch
#-----------------------------------------#

#The color map for dota.
my @g_colormap                    = ("red", "blue", "teal", "purple", "yellow", "orange", 
                                     "green", "pink", "grey", "light blue", "dark green", "brown");
#The team map for dota
my @g_teammap                     = ("Sentinel", "Scourge");

my $g_cmd_kick                    = "/usr/sbin/tcpkill -i $g_device -9 src host {ip} and dst host $g_gamehost";
my $g_cmd_kick2                   = "/usr/sbin/tcpkill -i $g_device -9 src host $g_gamehost and dst host {ip}";
my @g_data_prefixes               = ("FF", "F7");

#When we tab, it switches between this list and sets $g_activeindex
my @g_activelist                  = ("input", "list"); 

my @g_clientdisplay_default       = ("Computer - Sentinel", "Open", "Open", "Open", "Open", "Open", 
                                     "Computer - Scourge", "Open", "Open", "Open", "Open", "Open");
my @g_clientdisplay_colormap      = ("<L></B/16>", "<L></N/40>", "<L></N/56>", "<L></N/48>", "<L></B/32>", "<L></B/16>", 
                                     "<L></B/24>", "<L></B/48>", "<L></D/08>", "<L></B/40>", "<L></D/24>", "<L></N/32>");
my @g_clientdisplay_hightlightmap = ("</R/B/16>", "</R/N/40>", "</R/N/56>", "</R/N/48>", "</R/B/32>", "</R/B/16>", 
                                     "</R/B/24>", "</R/B/48>", "</R/D/08>", "</R/B/40>", "</R/D/24>", "</R/N/32>");

#-----------------------------------------#
# Other variables used by the program
#-----------------------------------------#

my $g_netmask       = '';
my $g_networkNumber = '';
my $g_operationMode = '';
my $g_pcap          = 0;
my $g_activeindex   : shared = 0;
my $g_bExit         : shared = 0;

my @g_clientdisplay = ("", "", "", "", "", "", "", "", "", "", "", "");

my @gd_clients = ();
my $g_events = Event->new ();
my $g_gi = Geo::IP->new (GEOIP_STANDARD);

#-----------------------------------------#
# Code
#-----------------------------------------#

#========================
sub Main ()
{
  #We don't want STDERR output to clutter the screen.
  open (STDERR, ">/dev/null");

  ResetEvents ();

  InitPcap ();
  InitCurses ();

  Output ("Loading...");

  CRefreshClientDisplay ();

  my $thread = threads->create ("CursesLoop", ""); 

  $g_coutput->clean (); #This is ok to do with one addline in the Swindow
  Output ("CWc3 by crenix has successfully loaded");
  Loop ();

  #Wait for thread to end by joining it.
  $thread->join ();

  Net::Pcap::close ($g_pcap);
  Cdk::end ();

  exit (0);
} Main ();

#========================
sub SystemHasDevice ($)
{
  my ($device) = @_;
  my $err = '';
  my $bFoundDevice = 0;
  my %devinfo;
  my @devs = Net::Pcap::findalldevs (\%devinfo, \$err);
  for my $dev (@devs) {
    if ($dev ne "lo" && $dev ne "any") {
      if ($dev eq $device) {
        $bFoundDevice = 1;
      }
    }
  }

  return $bFoundDevice;
}

#========================
sub InitPcap ()
{
  if (!SystemHasDevice ("$g_device")) {
    print "Error: $g_device was not found";
    exit;
  }

  my $err = '';
  if (Net::Pcap::lookupnet ($g_device, \$g_networkNumber, \$g_netmask, \$err) == -1) {
    die ("Error: lookupnet ($g_device): $err");
  }

  $g_pcap = Net::Pcap::open_live ($g_device, 1024, 1, 0, \$err)
    or die ("Can't open device $g_device: $err");

  my $filter;
  Net::Pcap::compile ($g_pcap, \$filter, "tcp port 6112", 1, $g_netmask);
  Net::Pcap::setfilter ($g_pcap, $filter);

  if (Net::Pcap::setnonblock ($g_pcap, 1, \$err) == -1) {
    die ("Error: setnonblock (): $err");
  }
}

#========================
sub InitCurses ()
{
  Cdk::init ();

  $g_clist = new Cdk::Scroll ('Title' => "", 'Highlight' => "</R/8>", 'Width' => 25, 'Height' => 14, 'Xpos' => "RIGHT", 'Ypos' => "TOP", 'List' => \@g_clientdisplay);
  $g_clist->draw ();

  $g_cinfo = new Cdk::Swindow ('Lines' => 100, 'Width' => 25, 'Height' => -20, 'Xpos' => "RIGHT", 'Ypos' => "15");
  $g_cinfo->draw ();

  $g_coutput = new Cdk::Swindow ('Lines' => 300, 'Width' => -27, 'Height' => -6, 'Xpos' => "LEFT", 'Ypos' => "TOP");
  $g_coutput->draw ();

  $g_cinput = new Cdk::Entry ('Label' => "#: ", 'Width' => -7, 'Height' => 0, 'Min' => 0, 'Max' => 256, 'Ypos' => "BOTTOM");
  $g_cinput->draw ();
}

#========================
sub Loop ()
{
  while (!$g_bExit) {
    Net::Pcap::dispatch ($g_pcap, -1, \&ProcessPacket, "");
  }
}

#========================
sub CursesLoop ()
{
  #Binds
  $g_clist->bind ('Key' => "KEY_TAB", 'Function' => sub {CB_Tab ();});
  $g_cinput->bind ('Key' => "KEY_TAB", 'Function' => sub {CB_Tab ();});
  $g_clist->bind ('Key' => "^c", 'Function' => sub {CB_CtrlC ();});
  $g_cinput->bind ('Key' => "^c", 'Function' => sub {CB_CtrlC ();});

  #PostProcess
  #$g_clist->postProcess ('Function' => sub {CB_ListPostProcess ();});

  while (!$g_bExit) {
    ActivateItem ();
  }
}

#========================
#Activates a certain ncurses item based on what $g_activeindex is set to.
sub ActivateItem ()
{
  if ($g_activelist[$g_activeindex] eq "input") {
    ActivateInput ();
  } 
  
  if ($g_activelist[$g_activeindex] eq "list") {
    ActivateList ();
  }
}

#========================
sub ActivateInput ()
{
  #Indicate where the user's input is directed to
  $g_cinput->set ('BoxAttribute' => "</B/32>");
  $g_clist->set ('BoxAttribute' => "");
  $g_cinput->draw ();
  $g_clist->draw ();

  my $info = $g_cinput->activate ();

  if (defined ($info)) {
    my $command = '', my $args = '';
    my $nCmdEnd = index ($info, "\x20");
    if ($nCmdEnd != -1) {
      $command = substr ($info, 0, $nCmdEnd);
      if ($nCmdEnd+1 <= length ($info)) {
        $args = substr ($info, $nCmdEnd+1);
      }
    } else {
      $command = $info;
    }

    DoCommand ($command, $args);

    $g_cinput->set ('Value' => "");
    $g_cinput->draw ();

    #We must reactivate the item after input is done.
    ActivateItem ();
  }
}

#========================
sub ActivateList ()
{
  #Indicate where the user's input is directed to
  $g_cinput->set ('BoxAttribute' => "");
  $g_clist->set ('BoxAttribute' => "</B/32>");
  $g_cinput->draw ();
  $g_clist->draw ();

  my $item = $g_clist->activate ();

  Output ("List item activated: $item");

  ActivateItem ();
}

#========================
sub CB_Tab ()
{
  $g_activeindex++;
  if ($g_activeindex > $#g_activelist) {
    $g_activeindex = 0;
  }

  ActivateItem ();
}

#========================
sub CB_CtrlC ()
{  
  Output ("Ctrl+C");
  $g_bExit = 1;
}

#========================
sub CB_ListPostProcess ()
{
  my ($size, $currentItem) = $g_clist->info ();

  #I decided to comment this out since it makes it very difficult to read
  #Set the correct highlight color
  #if (defined ($g_clientdisplay_hightlightmap[$currentItem])) {
  #  $g_clist->set ('Highlight' => $g_clientdisplay_hightlightmap[$currentItem]);
  #  $g_clist->draw ();
  #}

  #Reset window's contents
  #$g_cinfo->clean ();
  #This crashes the program. I think this was seriously overlooked when Cdk was created. You can
  #only clean an Swindow if there is only one line in it. If there are multiple lines this will crash.
  
  my $bFound = 0;
  foreach my $x (@gd_clients) {
    Output ("slot: " . $x->{slot} . ", currentItem: $currentItem");
    if (defined ($x->{slot}) && $currentItem == $x->{slot}) {
      $g_cinfo->addline ('Info' => "</B>name<!B>: "     . $x->{name})     if (defined ($x->{name}));
      $g_cinfo->addline ('Info' => "</B>ip<!B>: "       . $x->{ip})       if (defined ($x->{ip}));
      $g_cinfo->addline ('Info' => "</B>port<!B>: "     . $x->{port})     if (defined ($x->{port}));
      $g_cinfo->addline ('Info' => "</B>id<!B>: "       . $x->{id})       if (defined ($x->{id}));
      $g_cinfo->addline ('Info' => "</B>slot<!B>: "     . $x->{slot})     if (defined ($x->{slot}));
      $g_cinfo->addline ('Info' => "</B>color<!B>: "    . $x->{color})    if (defined ($x->{color}));
      $g_cinfo->addline ('Info' => "</B>teamname<!B>: " . $x->{teamname}) if (defined ($x->{teamname}));
      $g_cinfo->addline ('Info' => "</B>country<!B>: "  . $x->{country})  if (defined ($x->{country}));
      $g_cinfo->addline ('Info' => "");
      $bFound = 1;
      last;
    }
  }

  #This would only be relevant if we could clean the Swindow's contents. But we can't.
  if (!$bFound) {
    #$g_cinfo->addline ('Info' => "</B/16>No info found<!B!16>");
  }

  $g_cinfo->draw ();

  #We must reactivate the item or we will lose focus.
  ActivateItem ();
}

#========================
sub Output ($)
{
  my ($output) = @_;

  $output =~ s/^(Warning):/\<\/B\/32\>$1\<\!B\!32\>:/;
  $output =~ s/^(Error):/\<\/B\/16\>$1\<!B!16\>:/;
  $output =~ s/^(Freak error):/\<\/B\/16\>$1\<!B!16\>:/;

  $g_coutput->addline ('Info' => $output);
}

#========================
#This should be done in a better way somehow
sub CRefreshClientDisplay ()
{
  #We must save where our selected item was, then reset it.

  #Reset list first:
  for (my $x = 0; $x < $#g_clientdisplay + 1; $x++) {
    $g_clientdisplay[$x] = $g_clientdisplay_colormap[$x] . $g_clientdisplay_default[$x];
  }

  foreach my $x (@gd_clients) {
    if ($x->{connected}) {
      my $sDisplay  = $g_clientdisplay_colormap[$x->{slot}];
      $sDisplay    .= "$x->{name}";

      $g_clientdisplay[$x->{slot}] = $sDisplay;
    }
  }

  $g_clist->set ('Items' => \@g_clientdisplay);
  $g_clist->draw ();
}

#========================
sub ProcessPacket ($$$)
{
  my ($userdata, $header, $packet) = @_;
  my $match1, my $match2;
  my $data1, my $data2;
  
  my $ethpacket = NetPacket::Ethernet->decode ($packet);
  my $ippacket  = NetPacket::IP->decode ($ethpacket->{data});
  my $tcppacket = NetPacket::TCP->decode ($ippacket->{data});
  
  my $src_ip    = $ippacket->{src_ip};
  my $dest_ip   = $ippacket->{dest_ip};
  my $src_port  = $tcppacket->{src_port};
  my $dest_port = $tcppacket->{dest_port};
  my $flags     = $tcppacket->{flags};

  my $data      = $tcppacket->{data};

  my @datalist  = BreakUpData ($data);

  foreach $data (@datalist) {
    #++++++++++++++++++++++++++++++++++++++++++++++++++
    #When our server is sending data to battle.net:
    #++++++++++++++++++++++++++++++++++++++++++++++++++
#   if ($src_ip eq $g_gamehost && $dest_port == $g_battleport
#       && $flags & ACK && $flags & PSH) {

      #-------------------------------------
      #See when a game is created
      #-------------------------------------
      #Note: This is actually a status update packet. This packet
      #gets sent multiple times to the server. I'm guessing so it
      #can update its game list. The only way to tell afaik, is that
      #if the 9th byte is 0, then it is a create game. If it has any
      #other value it's a game status update packet.
      if (length ($data) >= 24) {
        $match1 = GetBinaryString ("FF 1C");
        $match2 = GetBinaryString ("00");    
        $data1 = substr ($data, 0, 2);
        $data2 = substr ($data, 8, 1);
      
        if ($data1 eq $match1 && $data2 eq $match2) {
          OnCreateGame ($data, $ippacket, $tcppacket);  
        }
      }

      #-------------------------------------
      #Sending a command to battle.net
      #-------------------------------------
      #The 2F at the end is the / ascii character.
      if (length ($data) >= 2) {
        $data1 = substr ($data, 0, 2);
        $match1 = GetBinaryString ("FF 0E");
   
        if ($data1 eq $match1) {
          OnSendCommand ($data, $ippacket, $tcppacket);
        }
      }

      #-------------------------------------
      #See if game was canceled/started by only the host if he's by himself
      #-------------------------------------
      if (length ($data) >= 4) {
        $data1 = substr ($data, 0, 4);
        $match1 = GetBinaryString ("FF 02 04 00");

        if ($data1 eq $match1) {
          OnOnlyHost ($data, $ippacket, $tcppacket);
        }
      }

      #-------------------------------------
      #See if game ended by us leaving
      #-------------------------------------
      if (length ($data) >= 2) {
        $data1 = substr ($data, 0, 2);
        $match1 = GetBinaryString ("FF 44");

        if ($data1 eq $match1) {
          OnEndGame ($data, $ippacket, $tcppacket);
        }
      }
#   }

    #++++++++++++++++++++++++++++++++++++++++++++++++++
    #When our server is receiving data from battle.net:
    #++++++++++++++++++++++++++++++++++++++++++++++++++
#   if ($dest_ip eq $g_gamehost && $src_port == $g_battleport
#       && $flags & ACK && $flags & PSH) { 

      #-------------------------------------
      #See if game was successfully created.
      #-------------------------------------
      #This does NOT only pertain to a successful/failed game creation.
      #Battle.net uses this same string to confirm other things as well.
      #E.g., if you refresh slots for example, a packet is sent to battle.net
      #with the game name and other information. If it was sent successfully, 
      #then a packet with the below binary string gets transmitted back to our server.
      #This is why I use the concept of events.
      if (length ($data) >= 8) {
        $data1 = substr ($data, 0, 8);
        $match1 = GetBinaryString ("FF 1C 08 00 00 00 00 00");
        $match2 = GetBinaryString ("FF 1C 08 00 01 00 00 00");

        if ($data1 eq $match1) {
          OnCreateGameSuccess ($data, $ippacket, $tcppacket);
        } elsif ($data1 eq $match2) {
          OnCreateGameFailed ($data, $ippacket, $tcppacket);
        }
      }
#   }

    #++++++++++++++++++++++++++++++++++++++++++++++++++
    #When our server is sending data to clients:
    #++++++++++++++++++++++++++++++++++++++++++++++++++
    #Most of this data is sent multiple times so that
    #each client can get it.
#   if ($src_ip eq $g_gamehost && $src_port == $g_gameport
#       && $flags & ACK && $flags & PSH) {

      #-------------------------------------
      #See if a game was started
      #-------------------------------------
      if (length ($data) >= 4) {
        $data1 = substr ($data, 0, 4);
        $match1 = GetBinaryString ("F7 0A 04 00");

        if ($data1 eq $match1) {
          OnStartGame ($data, $ippacket, $tcppacket);
        }
      }

      #-------------------------------------
      #See if game was canceled
      #-------------------------------------
      if (length ($data) == 9) {
        $data1 = substr ($data, 0, 9);
        $match1 = GetBinaryString ("F7 07 09 00 01 07 00 00 00");

        if ($data1 eq $match1) {
          OnCancelGame ($data, $ippacket, $tcppacket);
        }
      }

      #-------------------------------------
      #See if a game is being loaded
      #-------------------------------------
      if (length ($data) >= 4) {
        $data1 = substr ($data, 0, 4);
        $match1 = GetBinaryString ("F7 0B 04 00");

        if ($data1 eq $match1) {
          OnLoadGame ($data, $ippacket, $tcppacket);
        }
      }

      #-------------------------------------
      #Sending chat message
      #-------------------------------------
      if (length ($data) >= 2) {
        $data1 = substr ($data, 0, 2);
        $match1 = GetBinaryString ("F7 0F");

        if ($data1 eq $match1) {
          OnSendChatMsg ($data, $ippacket, $tcppacket);
        }
      }

      #-------------------------------------
      #Send initial client slot table (minus the 
      #person we're sending this to) and id
      #-------------------------------------
      if (length ($data) >= 2) {
        $data1 = substr ($data, 0, 2);
        $match1 = GetBinaryString ("F7 04");
        
        if ($data1 eq $match1) {
          OnSendInitialSlotTable ($data, $ippacket, $tcppacket);
        }
      }

      #-------------------------------------
      #Send updated slot table
      #-------------------------------------
      if (length ($data) >= 2) {
        $data1 = substr ($data, 0, 2);
        $match1 = GetBinaryString ("F7 09");
        
        if ($data1 eq $match1) {
          OnSendUpdateSlotTable ($data, $ippacket, $tcppacket);
        }
      }
      
      #-------------------------------------
      #Send users currently in the game, map path, and the slot table
      #-------------------------------------
      if (length ($data) >= 2) {
        $data1 = substr ($data, 0, 2);
        $match1 = GetBinaryString ("F7 06");
        
        if ($data1 eq $match1) {
          OnSendClientInfo ($data, $ippacket, $tcppacket);
        }
      }

      #-------------------------------------
      #Tell the client we opened/closed his/her slot
      #-------------------------------------
      if (length ($data) >= 8) {
        $data1 = substr ($data, 0, 8);
        $match1 = GetBinaryString ("F7 1C 08 00 0D 00 00 00");

        if ($data1 eq $match1) {
          OnSendSlotKickClient ($data, $ippacket, $tcppacket);
        }
      }
#   }

    #++++++++++++++++++++++++++++++++++++++++++++++++++
    #When our server is receiving data from clients:
    #++++++++++++++++++++++++++++++++++++++++++++++++++
#   if ($dest_ip eq $g_gamehost && $dest_port == $g_gameport
#       && $flags & ACK && $flags & PSH) {

      #-------------------------------------
      #Client changed slots
      #-------------------------------------
      if (length ($data) >= 6) {
        $data1 = substr ($data, 0, 6);
        $match1 = GetBinaryString ("F7 28 09 00 01 FF");

        if ($data1 eq $match1) {
          OnClientChangeSlots ($data, $ippacket, $tcppacket);
        }
      }

      #-------------------------------------
      #Client is attempting to connect
      #-------------------------------------
      if (length ($data) >= 2) {
        $data1 = substr ($data, 0, 2);
        $match1 = GetBinaryString ("F7 1E");
         
        if ($data1 eq $match1) {
          OnClientAttemptConnect ($data, $ippacket, $tcppacket);
        }
      }

      #-------------------------------------
      #Client left
      #-------------------------------------
      if (length ($data) >= 2) {
        $data1 = substr ($data, 0, 2);
        #$match1 = GetBinaryString ("F7 21 08 00 07 00 00 00");
        $match1 = GetBinaryString ("F7 21");
         
        if ($data1 eq $match1) {
          OnClientLeft ($data, $ippacket, $tcppacket);
        }
      }
#   }
  }
}

#========================
#This is here to break up data when multiple packets are combined. For example, sometimes
#one packet can contain current users who are connected, the slot table, and other info.
#Or, sometimes each of these are contained within their own packet. Luckily WC3's protocol
#allows us to easily parse all of this data out.
#Warning: This data must be parseable before calling this function or the program will crash.
sub BreakUpData ($)
{
  my ($data)    = @_;
  my $nDataSize = length ($data);
  my @datalist  = ();

  #Returns an empty list if this data isn't parseable.
  #All the checks are done here. So if we get passed this, we know that we can
  #parse it and not worry about errors.
  return @datalist if (!ParseableData ($data));

  #We say > 3 because the packet needs to be at least that size. Anything smaller isn't a
  #wc3 packet.
  while ($nDataSize > 3) {
    my $nReportedSize = GetDecimal (substr ($data, 2, 1));
    last if ($nReportedSize < 3);

    #Output ("Data size: $nDataSize, Reported size: $nReportedSize");
    #if ($nReportedSize < 5) {
    #  $printstring = GetHexString ($data, 0, 10);
    #  Output ("Weird: $printstring");
    #}

    my $extractedData = substr ($data, 0, $nReportedSize);
    push (@datalist, $extractedData);

    $data = substr ($data, length ($extractedData));
    $nDataSize = length ($data);
  }

  return @datalist;
}

#========================
#Data that we care about, basically.
sub ParseableData ($)
{
  my ($data) = @_;

  return 0 if (length ($data) < 4);

  my $compare = GetHexString ($data, 0, 1);

  foreach my $check (@g_data_prefixes) {
    if ($check eq $compare) {
      return 1;
    }
  }

  return 0;
}

#========================
sub GetHexString ($$$)
{
  my ($data, $offset, $length) = @_;
  
  my $string = unpack ('H*', substr ($data, $offset, $length));

  return uc ($string);
}

#========================
sub GetBinaryString ($)
{
  my ($hex) = @_;
  
  $hex = FormatHexString ($hex);
  my $string = pack ('H*', $hex);

  return $string;
}

#========================
#There is probably a better way to do this instead of unpacking to hex form, then
#converting the hex to decimal. But I just can't figure it out.
sub GetDecimal ($)
{
  my ($data) = @_;

  my $decimal = GetHexString ($data, 0, length ($data));
  $decimal = hex ($decimal);

  return $decimal;
}

#========================
sub FormatHexString ($)
{
  my ($hex) = @_;

  $hex =~ s/ //g;
  
  return uc ($hex);
}

#========================
sub PrintEvents ()
{
  Output ("Events:");
  Output ("createdgame\t\t$g_events->{createdgame}");
  Output ("creatinggame\t\t$g_events->{creatinggame}");
  Output ("startinggame\t\t$g_events->{startinggame}");
  Output ("startedgame\t\t$g_events->{startedgame}");
  Output ("canceledgame\t\t$g_events->{canceledgame}");
  Output ("endedgame\t\t$g_events->{endedgame}");
}

#=========================
sub ResetEvents ()
{
  $g_events->{createdgame} = 0;
  $g_events->{creatinggame} = 0;
  $g_events->{startinggame} = 0;
  $g_events->{startedgame} = 0;
  $g_events->{canceledgame} = 0;
  $g_events->{endedgame} = 0;
}

#========================
#This function will update the client list based on ip and port.
#It will also create a new entry if it doesn't exist.
sub UpdateClientByIpPort ($$%)
{
  my ($ip, $port, %clientdata) = @_;

  #Add the ip and port to the client data just in case
  #the client isn't in the list and we have to add him.
  $clientdata{'ip'} = $ip;
  $clientdata{'port'} = $port;

  my $client_ref = GetClientRefByIpPort ($ip, $port);
  if (!$client_ref) {
    AddNewClient (%clientdata);
  } else {
    #Update the client's data
    UpdateClientsData ($client_ref, %clientdata);
  }
}

#========================
#This will update an entry based on a client id.
#It will also create a new entry if it doesn't exist.
sub UpdateClientById ($%)
{
  my ($id, %clientdata) = @_;

  #Add the id to the client data just in case
  #the client isn't in the list and we have to add him.
  $clientdata{'id'} = $id;

  my $client_ref = GetClientRefById ($id);
  if (!$client_ref) {
    AddNewClient (%clientdata);
  } else {
    #Update the client's data
    UpdateClientsData ($client_ref, %clientdata);
  }
}

#========================
#Adds a new client and returns a reference to it.
#When it adds a client, it sets the default data, then
#updates the client's data with the clientdata passed.
sub AddNewClient (%)
{
  my (%clientdata) = @_;

  #Create the client structure add it to the list
  my $client_ref = \Client->new ();
  push (@gd_clients, $$client_ref);

  #Set all the default variables for this client. This could probably be
  #done in a better way with some sort of Merge function.
  my %default_clientdata = 
  (
    "name"      => 'unknown',
    "id"		    => 'unknown',
    "color"     => 'unknown',
    "slot"      => 'unknown',
    "team"      => 'unknown',
    "teamname"  => 'unknown',
    "ip"        => 'unknown',
    "port"      => 'unknown',
    "flags"     => 0,
    "connected" => 0, #They haven't connected until we send them the initial slot table
    "country"   => 'unknown',
  );
  UpdateClientsData ($client_ref, %default_clientdata);

  #Now update the client with the clientdata passed to this function
  UpdateClientsData ($client_ref, %clientdata);

  return $client_ref;
}

#========================
#Returns a reference to a Client object based on ip and port
sub GetClientRefByIpPort ($)
{
  my ($ip, $port) = @_;

  my $client_ref = 0;
  for (my $x = 0; $x < $#gd_clients+1; $x++) {
    if ($gd_clients[$x]->{ip} eq $ip && $gd_clients[$x]->{port} eq $port) {
      $client_ref = \$gd_clients[$x];
    }
  }

  return $client_ref;
}

#========================
#Returns a reference to a Client object based on id
sub GetClientRefById ($)
{
  my ($id) = @_;

  my $client_ref = 0;
  for (my $x = 0; $x < $#gd_clients+1; $x++) {
    if ($gd_clients[$x]->{id} eq $id) {
      $client_ref = \$gd_clients[$x];
    }
  }

  return $client_ref;
}

#========================
#This function takes a reference to a $client.
#%clientdata is passed by value
sub UpdateClientsData ($%)
{
  my ($client_ref, %clientdata) = @_;

  foreach my $key (keys (%clientdata)) {
    $$client_ref->{$key} = $clientdata{$key};
  }
}

#========================
sub RemoveClientByIpPort ($$)
{
  my ($ip, $port) = @_;

  my $client = 0;
  for (my $x = 0; $x < $#gd_clients+1; $x++) {
    if ($gd_clients[$x]->{ip} eq $ip && $gd_clients[$x]->{port} eq $port) {
      splice (@gd_clients, $x, 1);
      return;
    }
  }
}

#========================
sub PrintClientList ()
{
  foreach my $x (@gd_clients) {
    if ($x->{connected}) {
      Output ("-----------------------");
      Output ("name:     " . $x->{name} . "");
      Output ("ip:       " . $x->{ip} . "");
      Output ("port:     " . $x->{port} . "");
      Output ("id:       " . $x->{id} . "");
      Output ("slot:     " . $x->{slot} . "");
      Output ("color:    " . $x->{color} . "");
      Output ("team:     " . $x->{team} . "");
      Output ("teamname: " . $x->{teamname} . "");
    }
  }

  if (scalar (@gd_clients) == 0) {
    Output ("No clients in list to print.");
  }
}

#========================
sub ClearClientList ()
{
  splice (@gd_clients, 0);
}

#========================
sub RunCommand ($$)
{
  my ($sleep, $cmd) = @_;

  my $pid = fork ();
  if (!$pid) {
    open (STDERR, ">/dev/null");
    open (STDOUT, ">/dev/null");
    system ("./maxtime $sleep $cmd");
    exit;
  }
}

#========================
#Format:
#FF 1C [size of data] 00 [private/public] 00 00 00 00 00 00 00 01 [??] 49 00
#FF 03 00 00 00 00 00 00 [game name]
#
#private:  11
#public:   10
sub OnCreateGame ($$$)
{
  my ($data, $ippacket, $tcppacket) = @_;

  #Here, we don't test if we're in a game already or not. The reason is sometimes this
  #program might not catch when a game ends. If that happens, and you create another game,
  #this will not work. So, since this is the only type of packet sent when a game is created,
  #we might as well reset all the events and create a game anyways.

  my $gametype_data = substr ($data, 4, 1);

  #Game name -
  #At offset 24 is the game name. It is terminated with 0.
  my $gamename = substr ($data, 24);
  $gamename = substr ($gamename, 0, index ($gamename, "\x00"));

  #Type of game - 10 for public, 11 for private
  my $gametype = "Public";
  if ($gametype_data eq GetBinaryString ("11")) {
    $gametype = "Private";
  }

  #We're in host mode. Which means we created a game.
  $g_operationMode = "host";

  ClearClientList ();
  ResetEvents ();
  $g_events->{creatinggame} = 1;

  Output ("Game: Create [$gametype] $gamename");
}

#========================
sub OnCreateGameSuccess ($$$)
{
  my ($data, $ippacket, $tcppacket) = @_;

  #Only do this once. The reason is explain in ProcessPacket().
  return if (!$g_events->{creatinggame});

  ResetEvents ();
  $g_events->{createdgame} = 1;

  CRefreshClientDisplay ();

  Output ("Success: Game created");
}

#========================
sub OnCreateGameFailed ($$$)
{
  my ($data, $ippacket, $tcppacket) = @_;

  #Only do this once. The reason is explain in ProcessPacket().
  return if (!$g_events->{creatinggame});
  
  ResetEvents ();

  Output ("Failed: Game name already in use");
}

#========================
sub OnStartGame ($$$)
{
  my ($data, $ippacket, $tcppacket) = @_;

  return if ($g_events->{startinggame} || $g_events->{startedgame});

  $g_events->{startinggame} = 1;

  Output ("Game: started");
}

#========================
sub OnCancelGame ($$$)
{
  my ($data, $ippacket, $tcppacket) = @_;

  return if (!$g_events->{createdgame} || $g_events->{canceledgame});

  ResetEvents ();
  $g_events->{canceledgame} = 1;

  Output ("Game: canceled");
}

#========================
sub OnEndGame ($$$)
{
  my ($data, $ippacket, $tcppacket) = @_;

  return if (!$g_events->{createdgame} || $g_events->{endedgame} || $g_events->{canceledgame});

  ResetEvents ();
  $g_events->{endedgame} = 1;

  Output ("Game: ended");
}

#========================
sub OnLoadGame ($$$)
{
  my ($data, $ippacket, $tcppacket) = @_;

  return if (!$g_events->{startinggame});

  $g_events->{creatinggame} = 0;
  $g_events->{startinggame} = 0;
  $g_events->{startedgame} = 1;
  $g_events->{canceledgame} = 0;

  Output ("Game: loading");
}

#========================
sub OnOnlyHost ($$$)
{
  my ($data, $ippacket, $tcppacket) = @_;

  if ($g_events->{createdgame} && $#gd_clients + 1 == 0) {
    Output ("Game started/canceled and only host is there");
  }
}

#========================
#Format: FF 0E [size of data] 00 [/] [command]
sub OnSendCommand ($$$)
{
  my ($data, $ippacket, $tcppacket) = @_;

  my $command = '', my $args = '';
  my $fullcommand = substr ($data, 5);
  $fullcommand = substr ($fullcommand, 0, index ($fullcommand, "\x00"));

  my $nCmdEnd = index ($fullcommand, "\x20");
  if ($nCmdEnd != -1) {
    $command = substr ($fullcommand, 0, $nCmdEnd);
    if ($nCmdEnd+1 <= length ($fullcommand)) {
      $args = substr ($fullcommand, $nCmdEnd+1);
    }
  } else {
    $command = $fullcommand;
  }

  #Check if the command is in the dummy list. If it is, just return.
  #These are mostly the commands from banlist.
  my @dummylist = ("ban", "banlast", "last", "info", "check", "checkall", "checkchannel", "nodl",
                   "autokick", "ping", "pingall", "from", "anyfrom", "fromall", "phrase", "np", "lm",
                   "rank", "notwhispered", "nw", "reserve", "showreservation", "sr", "fromlast", "pinglast",
                   "whois", "squelch");
  foreach my $x (@dummylist) {
    if ($x eq $command) {
      return;
    }
  }

  DoCommand ($command, $args);
}

#========================
#First parameter is the command. Second are the arguments.
sub DoCommand ($$)
{
  my ($command, $args) = @_;

  if ($command eq "kick") {
    foreach my $x (@gd_clients) {
      my $namematch = lc ($args);
      if (lc ($x->{name}) =~ /$namematch/) {
        next if ($x->{id} == 1); #Don't kick the host.

        $x->{flags} = "k";
        Output ("Kicking: \"" . $x->{name} . "\"");
        my $realcmd = $g_cmd_kick;
        $realcmd =~ s/\{ip\}/$x->{ip}/;
        RunCommand (15, $realcmd);
        return;
      }
    }

    Output ("Error: Could not kick. Client \"" . $args . "\" not found");
  } elsif ($command eq "kick2") {
    foreach my $x (@gd_clients) {
      my $namematch = lc ($args);
      if (lc ($x->{name}) =~ /$namematch/) {
        next if ($x->{id} == 1); #Don't kick the host.

        $x->{flags} = "k";
        Output ("Kicking: \"" . $x->{name} . "\"");
        my $realcmd = $g_cmd_kick2;
        $realcmd =~ s/\{ip\}/$x->{ip}/;
        RunCommand (15, $realcmd);
        return;
      }
    }

    Output ("Error: Could not kick2. Client \"" . $args . "\" not found");
  } elsif ($command eq "listclients" || $command eq "listplayers" || $command eq "clientlist") {
    PrintClientList ();
  } elsif ($command eq "listevents") {
    PrintEvents ();
  } elsif ($command eq "test") {
    my %clientdata = 
    (
      "name"      => "CrEnIx",
      "connected" => 1,
      "slot"      => 1,
    );

    UpdateClientById (1, %clientdata);
    CRefreshClientDisplay ();
  } elsif ($command eq "save") {
    open (GAYDUMP, ">dump.pm");
    print GAYDUMP Dumper(\@gd_clients);
    close (GAYDUMP);
  } elsif ($command eq "load") {
    do ("dump.pm");
  } else {
    Output ("Unknown command: $command");
  }
}

#========================
#Format: F7 0F [size of data] 00 [# of clients we send the msg to]
#        [client id] [client id] ... [??] 10 [chat msg]
sub OnSendChatMsg ($$$)
{
  my ($data, $ippacket, $tcppacket) = @_;
}

#========================
#Format: F7 28 09 00 01 FF [client id] 11 [team id (0/1)]
sub OnClientChangeSlots ($$$)
{
  my ($data, $ippacket, $tcppacket) = @_;
}

#========================
#I'm still not exactly certain of this packet. Sometimes it can come with a full slot table,
#and sometimes that is just missing. What's most important though, is that the ID of the
#person we're sending this packet to is contained in here. Even though the packet size can
#vary, the ID is always located 17 bytes from the end.
#
#Format #1: 
#F7 04 [size of data] 00 73 00 [number of entries]
#
#[The slot table described in OnSendUpdateSlotTable(), the inner 12 lines]
#
#[??] [??] [??] [??] [??] [??] [client id] 
#[??] [??] [??] [??] [??] [??] [??] [??] 00 00 00 00 00 00 00 00
#
#Format #2:
#F7 04 [size of data] 00 00 00 [client id] 
#[??] [??] [??] [??] [??] [??] [??] [??] 00 00 00 00 00 00 00 00
#
#client id:  This is the client id of the client we're sending this packet to.
sub OnSendInitialSlotTable ($$$)
{
  my ($data, $ippacket, $tcppacket) = @_;

  my $id_reverse_offset = 17;

  my $id = GetDecimal (substr ($data, length ($data) - $id_reverse_offset, 1));

  my %clientdata = 
  (
    "id"        => $id,
    "connected" => 1,
  );

  UpdateClientByIpPort ($ippacket->{dest_ip}, $tcppacket->{dest_port}, %clientdata);

  my $client_ref = GetClientRefById ($id);
  if ($client_ref) {
    Output ("Joined: \"". $$client_ref->{name} . "\" (" . $$client_ref->{country} . ")");
    CRefreshClientDisplay ();

    #Maybe we'll do an automatic kick later.
    my $bFound = 0;
    foreach my $code (@g_allowed_countries) {
      if ($$client_ref->{country} eq $code) {
        $bFound = 1;
      }
    }
    if (!$bFound) {
      Output ("Warning: Client \"". $$client_ref->{name} . "\" is not from: " . join (", ", @g_allowed_countries));
    }
  } else {
    Output ("Freak error: Can't get the client data of person who joined");
  }
}

#========================
#Format:
#F7 09 [size of data] 00 73 00 [number of entries] 
#[client id] [FF/64] [slot status] [kind] [team id] [slot id - 00] [??] 01 64 
#...
#number of entries times
#...
#5A 44 EA 06 03 0C
#
#number of entries: The number of entries in the table, 0C for 12 entries
#client id:         00 for no one in the slot, the client's id if they are in this slot
#kind:              00 for player, 01 for computer
#teamid:            00 for nightelf/sentinel, 01 for undead/scourge
#slot status:       00 for open, 01 for closed, 02 for occupied
sub OnSendUpdateSlotTable ($$$)
{
  my ($data, $ippacket, $tcppacket) = @_;

  my $entries_offset = 6;
  my $entries_start_offset = 7;
  my $entry_size = 9;

  my $id_offset = 0;
  my $slotstatus_offset = 2;
  my $kind_offset = 3;
  my $teamid_offset = 4;
  my $slotid_offset = 5;

  my $entries = GetDecimal (substr ($data, $entries_offset, 1));

  #Output ("SlotTable entries: $entries");
  
  for (my $x = 0; $x < $entries; $x++) {
    my $entry = substr ($data, $x * $entry_size + $entries_start_offset, $entry_size);

    my $id = GetDecimal (substr ($entry, $id_offset, 1));
    next if ($id == 0);

    my $slotstatus = GetDecimal (substr ($entry, $slotstatus_offset, 1));
    my $kind = GetDecimal (substr ($entry, $kind_offset, 1));
    my $teamid = GetDecimal (substr ($entry, $teamid_offset, 1));
    my $slotid = GetDecimal (substr ($entry, $slotid_offset, 1));

    #Output ("Entry: id = $id, slotstatus = $slotstatus, kind = $kind, teamid = $teamid, slotid = $slotid");

    my %clientdata =
    (
      "team"     => $teamid,
      "slot"     => $slotid,
      "color"    => $g_colormap[$slotid],
      "teamname" => $g_teammap[$teamid],
    );

    UpdateClientById ($id, %clientdata);
  }

  CRefreshClientDisplay ();
}

#========================
#Format:
#F7 06 [size of data] 00 [??] 00 00 00 
#[client id] [client name] 00 01
#
#No slot info is sent in this.
sub OnSendClientInfo ($$$)
{
  my ($data, $ippacket, $tcppacket) = @_;

  my $id_offset = 8;
  my $name_offset = 9;

  my $id = GetDecimal (substr ($data, $id_offset, 1));

  my $name = substr ($data, $name_offset);
  $name = substr ($name, 0, index ($name, "\x00"));

  my %clientdata = 
  (
    "name"      => $name,
    "connected" => 1,
  );

  UpdateClientById ($id, %clientdata);
  CRefreshClientDisplay ();
}

#========================
#Format: F7 1E [size of data] 00 [??] 00 00 00 00 00 00 00 00 
#        [??] [??] [??] 00 00 00 [client name] 
sub OnClientAttemptConnect ($$$)
{
  my ($data, $ippacket, $tcppacket) = @_;

  #We started the game, so we're not accepting anymore client connects.
  return if ($g_events->{startinggame} || $g_events->{startedgame});

  my $name_offset = 19;

  my $name = substr ($data, $name_offset);
  $name = substr ($name, 0, index ($name, "\x00"));

  my %clientdata = 
  (
    "name"      => $name,
    "country"   => $g_gi->country_code_by_addr ($ippacket->{src_ip}),
  );

  UpdateClientByIpPort ($ippacket->{src_ip}, $tcppacket->{src_port}, %clientdata);

  #We're in client mode. Which means we joined a game.
  if ($ippacket->{src_ip} eq $g_gamehost) {
    $g_operationMode = "client";
  }

  #Output ("Connection attempt: \"$name\"");
}

#========================
sub OnSendSlotKickClient ($$$)
{
  my ($data, $ippacket, $tcppacket) = @_;

  foreach my $x (@gd_clients) {
    if ($ippacket->{dest_ip} eq $x->{ip} && $tcppacket->{dest_port} eq $x->{port}) {
      RemoveClientByIpPort ($ippacket->{dest_ip}, $tcppacket->{dest_port});

      Output ("Kicked: \"$x->{name}\" by slot");
      CRefreshClientDisplay ();
      return;
    }
  }
}

#========================
sub OnClientLeft ($$$)
{
  my ($data, $ippacket, $tcppacket) = @_;

  foreach my $x (@gd_clients) {
    if ($ippacket->{src_ip} eq $x->{ip} && $tcppacket->{src_port} eq $x->{port}) {
      RemoveClientByIpPort ($ippacket->{src_ip}, $tcppacket->{src_port});

      Output ("Left: \"$x->{name}\"");
      CRefreshClientDisplay ();
      return;
    }
  }
}
