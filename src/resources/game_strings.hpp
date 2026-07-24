#pragma once
// ReSharper disable CppClangTidyClangDiagnosticInvalidUtf8
// ReSharper disable CommentTypo



struct CaptionOverride
{
    uint32_t ps2_crc32;
    const char* replacement;
};

//hash is the crc32 of the full original string, linebreak bytes & all.
constexpr CaptionOverride kMGS2CaptionTypoFixes[] =
{
                        //stringdb

    { 0xc64ffffc, "\"Do not trouble yourself over it,\" said a \nvoice behind Snake's back. It was \nGurlukovich. He held a pistol in his \nhand.\n\"Why were you using Meryl?\" Snake \ndemanded, fighting to control his anger.\n\"USING her? I merely told her what \nshe wanted to know.\"\n\"I don't think so.\"\nA slight smile crossed Gurlukovich's \nface. It was the smile of a master \nstrategist, a smile that told of lies, \nmanipulation, and betrayal. \"Perhaps I \ndid embellish the truth a bit. After all, \npeople will believe what they want to \nbelieve.\"\n\"Is this your idea of revenge on the \nColonel?\"\n\"Of course. He deserves to suffer. I will \nmake him squirm in agony until his pain \nis as great as the suffering of all of the \nmen that I have had to bury, and all of \nthe widows and orphans they left \nbehind.\"\nGurlukovich hated Campbell. Campbell \nhated Gurlukovich. Hatred spawned \nhatred in kind.\n\"That is why I chose her. Is she not \nthe most important person in \nCampbell's life? This I heard from \nMatt... as he lay dying.\"\n\"So it was you that killed Matt?\"\nGurlukovich's eyes darkened for an \ninstant. \"I see my reputation precedes \nme. I simply gave him that which he \ndesired. He was only waiting for \nsomeone to kill him. That is the kind of \nman he was.\" For a moment, it almost \nlooked as if Gurlukovich were truly \ngrieving the loss of an old friend. \nPerhaps his grief was real. But the look \nsoon passed.\n\"I will not hand the Metal Gear over to \nthe Elderan guerrillas. My men and I \nhave other plans for it. The machine \nitself may be a wreck, but all we really \nneed to do is recover the nuclear \nwarheads and the launching system. A \nnuclear-equipped ballistic missile is still \na precious commodity, even in this day \nand age.\"\nSnake gritted his teeth. Gurlukovich \nlaughed.\n\"Do not be so angry with me. This is \nMatt Campbell's dying wish. This was \nhis intention from the beginning.\"\n\"What do you mean?\"\n\"If you do not believe me, then ask him \nyourself.\"\nGurlukovich aimed his gun at Snake.\n\"In hell!\"\nThe sound of a gunshot ripped through \nthe air. \n|\nGurlukovich fell to the ground. \nThe shot had come from Meryl.#C0\n" },
    { 0x6cd9561f, "\"Snake, I have a favor to ask,\" said \nOtacon.\nSnake's face creased into a frown. \nOtacon's \"favors\" always meant trouble, \nusually for Snake.\n\"Forget it. I don't know what you're up \nto this time, but I'm through running \naround the country on some half-baked \ncrusade I don't even understand.\"\n\"It's nothing like that. It's... it's my \nsister.\"\n\"Your sister?\"\n\"Yeah. Her name's Emma.\" Otacon's \nvoice suddenly became grave as he \nbegan to relate his story. He'd recently \ndiscovered the whereabouts of his \nlong-estranged stepsister. Apparently, \nshe was working as a systems \nprogrammer at an ocean cleanup facility \nin Manhattan Bay called \"Big Shell.\"\n\"Wait a minute. Big Shell... isn't that\nwhere...?\"\n\"Exactly. So you've heard the rumors, \ntoo.\"\nThere had recently been a string of \naccidental deaths among the employees \nof Big Shell. A number of rumors were \ncirculating about the cause of these \ndeaths - there was a serial killer on the \nloose, the employees were going \nstir-crazy, the whole place was haunted. \nBig Shell had gone from being a \nsymbol of environmental responsibility \nto being a lucrative source of tabloid \ngossip.\n\"I'm worried about Emma. Will you go \nand check on her for me?\"\nSnake waved his hand in dismissal. \n\"Go ask the police or something.\"\n\"You know they wouldn't do anything \nabout it.\"\n\"And you assumed that I would?\"\n\"I've got a bad feeling about this thing. \nJust go and check for me, please?\"\n\"You go.\"\n\"Come on!\"\n\"No way!\"\n\"I'm telling you, Snake, I've got a \nREALLY bad feeling about this! Please, \nI'm begging you!\"\n\"Leave me alone!\"\n|\nSix hours later, Snake found himself \nstanding on the heliport of Big Shell. \nHe watched as a group of armed men \nbegan to take over the facility before \nhis eyes. Snake hid himself behind a \ncrate. Just then, Otacon called on the \nCodec.\n\"...You think they're some kind of \nterrorists?\" he asked.\n\"I dunno. But it's pretty clear that \nthe situation is a little more dangerous \nthan before. Congratulations. Looks like \nyou guessed right.\"\n\"That's not exactly reassuring. I hope \nEmma's still okay....\"\n\"Yeah, me too,\" said Snake. His eye \ndrifted over to a Harrier jet parked on \nthe heliport. \"What about that Harrier? \nDoes it belong to them?\"\n\"No, that belongs to the Marines. \nApparently they made an emergency \nlanding here three days ago, after a \nproblem occurred during a training \nexercise.\"\nSnake nodded. \"First things first. We'd \nbetter find out what exactly is going on \nhere. I'm going in.\"\n\"Roger. Be careful in there. Oh, and \nSnake...\"\n\"What?\"\n\"If you do happen to meet Emma, don't \nmention my name, okay?\"\n\"Why not?\"\n\"Well... we haven't seen each other in a \nlong time... and it's all kind of sudden...\"\nOtacon hesitated. Snake realized that \nthere was more to the situation than \nOtacon was willing to let on. He decided \nto let it drop as he headed inside Big \nShell.#C0\n" },
    { 0x42e7c3c9, "- Revolver Ocelot\n\343\200\200An ex-Spetsnaz, also known as\n\"Shalashaska.\" After the collapse of the\nSoviet Union, he found lucrative contracts\nas a mercenary in conflict-ridden regions\nthroughout the world. His activities led to his\nrecruitment by the U.S. government, and\nhis entry into FOXHOUND. As his code name\nindicates, he is a brilliant marksman whose\npreferred weapon is a revolver.\n" },
    { 0xfcc0d0b4, "A pesar de su tama\303\261o, puedes confiar\nen el SOCOM.\n" },
    { 0x32fe8e76, "AB Connecting Bridge" },
    { 0xae1a8807, "Abbiamo perso il contatto col ponte,|livello 5. Unit\303\240 di rinforzo, andate.  " },
    { 0x0b658d67, "Aber ich werde erreichen, was unser Vater immer\nertr\303\244umte, aber nie schaffte. So werde ich ihn\nt\303\266ten, indem ich ihn \303\274bertreffe.\" Liquid\nbeendete seine Erkl\303\244rung, indem er in das\nCockpit von Metal Gear sprang. Snake gab einen\nFeuersto\303\237 mit seiner SOCOM ab, aber die Kugeln\nprallten an der Panzerung von Metal Gear ab.\n\343\200\200\"Verdammt!\" Snake knirschte mit den Z\303\244hnen,\nals Liquid h\303\266hnisch aus dem Cockpit rief.\n\343\200\200\"Snake, Du kannst Dich gl\303\274cklich sch\303\244tzen.\n" },
    { 0x57e9ed3d, "Aft Deck" },
    { 0x38eea5b1, "As Snake emerged onto the Shell 1-2 \nconnecting bridge, he got a call on \nthe Codec.\n\"This is Snake.\"\n\"Don't give me that 'this is Snake' \nroutine!\" Otacon was furious. \n\"Are you just gonna leave Emma there \nto fend for herself? You ought to be \nashamed of yourself! Get back there \nright away!\"#C0\n" },
    { 0x815975aa, "As Snake emerged onto the Shell 1-2 \nconnecting bridge, he got a call on \nthe Codec.\"This is Snake.\"\n\"Don't give me that 'this is Snake' \nroutine!\" Otacon was furious. \"Where \ndo you think you're going, anyway? \nEmma is in Strut F! Get over there \nnow!\"#C0\n" },
    { 0xb3f0429c, "avait pr\303\251vu qu'on tenterait de lib\303\251rer Baker.\n\343\200\200Selon Naomi Hunter, l'ancienne directrice du\nprogramme de manipulation g\303\251n\303\251tique de\nFOXHOUND, Revolver Ocelot \303\251tait un ancien\nmembre de Spetsnaz. Il avait fait partie de\nOMON (Otryad Militsii Osobogo Naznacheniya, la\nbrigade anti-\303\251meute du minist\303\250re de l'Int\303\251rieur,\n\303\251galement connue sous le nom de 'B\303\251rets noirs')\npuis du SVR (les Services secrets russes), un\nservice qui avait succ\303\251d\303\251, apr\303\250s la chute du bloc\n" },
    { 0x4ab543e4, "BC Connecting Bridge" },
    { 0x65ecf3e9, "CD Connecting Bridge" },
    { 0x0033db4b, "celle a Lubyanka. Ora sapevamo che i terroristi\npossedevano entrambe le password. La\nsituazione non poteva essere pi\303\271 disperata.\n\343\200\200La risposta di Baker alle domande di Snake\ncirca il codice di emergenza per fermare il\nlancio era altrettanto demoralizzante. Baker\nl'aveva comunicato a una donna soldato che\nsi trovava nella sua stessa cella e che aveva\nrifiutato di unirsi all'ammutinamento. Ho sentito\nSnake mormorare improvvisamente.\n" },
    { 0x7603aec1, "che Snake potesse liberarlo, si \303\250 trovato faccia\na faccia con il creatore di questa trappola: un\nagente di FOXHOUND chiamato Revolver Ocelot.\nSembra che, dopo aver avuto notizia dell'arrivo\ndi Snake, questi avesse previsto il tentativo di\nliberare Baker.\n\343\200\200A detta di Naomi Hunter, Revolver Ocelot,\nex direttore del programma di manipolazione\ngenetica per FOXHOUND, in passato era uno\nSpetsnaz. In seguito \303\250 passato all'OMON\n" },
    { 0x0651441b, "comprendre. Quelqu'un qui a eu la chance de\ntuer son propre p\303\250re ne pourrait pas le\ncomprendre ! Tu as m\303\252me r\303\251ussi \303\240 m'\303\264ter ce\nprivil\303\250ge. Mais moi, je vais concr\303\251tiser son r\303\252ve...\nCelui qu'il n'a jamais r\303\251ussi \303\240 accomplir. C'est\ncomme \303\247a que je vais le tuer... en le surpassant.\"\nSur ces mots, Liquid se glissa dans le cockpit de\nMetal Gear. Snake tira \303\240 l'aide de son SOCOM,\nmais les balles ricoch\303\250rent sur l'armure de Metal\nGear.\n" },
    { 0xabd5553a, "Cuchillo scout con sorpresa...|\302\277eres una Spetsnaz?" },
    { 0x0fc594fc, "Das Fahrtenmesser sah ganz\nnach Spetsnaz aus, aber.\n" },
    { 0x534ad522, "DE Connecting Bridge" },
    { 0x03207bf5, "Deck-2, Port" },
    { 0xefefb222, "Deck-2, Starboard" },
    { 0xe53fae22, "Deck-A, Crew's Lounge" },
    { 0x58eb21f8, "Deck-A, Crew's Quarters" },
    { 0xb26dfc9a, "Deck-B, Crew's Quarters" },
    { 0x5d3f4a7b, "Deck-C, Crew's Quarters" },
    { 0xbc11401f, "Deck-D, Crew's Quarters" },
    { 0x0bb80575, "Deck-E, The Bridge" },
    { 0x63286884, "Deine Pistole ist eine SOCOM,\n" },
    { 0xc926894d, "Deine SOCOM ist mit einem\nSchalld\303\244mpfer ausger\303\274stet.\n" },
    { 0x61f5f5fd, "Des renforts se dirigent vers la salle|des moteurs. Prot\303\251gez l'entr\303\251e tribord." },
    { 0x8e965ffa, "Destroy all targets and head to the goal!\nObserve the targets' route and determine\nthe point where you can destroy many \nat once.\n" },
    { 0x76b9fae7, "Destroy all targets and head to the goal!\nUnequipping right after firing will cause\nthe missile to explode. Be careful.\n" },
    { 0xe1a3d024, "Dolph contacted me as soon as I \narrived at Strut D. He had apparently \nmanaged to get a hold of O'Brien. \nO'Brien had sealed off the door to the \nShell 1-2 connecting bridge from \nStrut D to prevent Vamp from getting \nthrough. He'd also reorganized the \nsurviving SEALs to defend Strut G. \nDolph said:\n\"Snake, join the SEALs. Vamp is the \nenemy. Those men might be SEALs, \nbut I doubt they can stop him on their \nown.\" \n\"But how can I get to Shell 2? The \nShell 1-2 connecting bridge is closed \noff.\"\n\"There is another way. Go across the \noil fence in the lower part of Big Shell. \nUse the ladder on B1 of Strut E to get \ndown to the oil fence. You should be \nable to get to Strut L of Shell 2 by \ncrossing it.\" \nVamp must have already left for \nShell 2. I needed to hurry.#C0\n" },
    { 0xb6711008, "during his days with the Spetsnaz. In other\nwords, he was an expert in torture. There was\nno way that the weapons technology\nexecutive, an untrained civilian, could with-\nstand the techniques of coercion developed in\nthe cells of Lubyanka. We now had con-\nfirmation that the terrorists possessed both\nlaunch keys. The situation was more desperate\nthan ever.\n\343\200\200 Baker's response to Snake's queries about\n" },
    { 0x957b68cd, "EF Connecting Bridge" },
    { 0x4de08bae, "Ein Fahrtenmesser mit \303\234berraschung,|bist Du von Spetsnaz?" },
    { 0xfca938d6, "El SOCOM tiene una espectiva de vida\nm\303\255nima de unas 6.000 rondas,\n" },
    { 0xea4933ea, "Emma has been captured. With Otacon's \nhelp, I found out that Emma is somewhere \nin Strut F. I'll do anything it takes to rescue \nEmma.\n" },
    { 0x129c824b, "Emma told me that I must open the door to \nthe Shell 1-2 connecting bridge to go to \nShell 2 where the hostages are kept. I must \naccess the node in Strut B to do so.\n" },
    { 0x20a78344, "Emma told me that I must open the door to \nthe Shell 1-2 connecting bridge to go to \nShell 2, where the hostages are kept. \nI must access the node in Strut B to do so.\n" },
    { 0xe5ba6fad, "Engine Room" },
    { 0xfabcd279, "Es un SOCOM.\n" },
    { 0xac353680, "expert en tortures en tous genres. Face \303\240 des\ntechniques de coercition d\303\251velopp\303\251es dans les\ncellules de Lubyanka, un cadre sup\303\251rieur\nsp\303\251cialis\303\251 dans la vente d'armes, un civil sans\nentra\303\256nement, n'avait aucune chance de r\303\251sister.\nNous savions maintenant que les terroristes\ns'\303\251taient empar\303\251s des deux mots de passe\nn\303\251cessaires au lancement d'une frappe nucl\303\251aire.\nLa situation \303\251tait plus d\303\251sesp\303\251r\303\251e qu'on ne le\npensait.\n" },
    { 0x70ecae4c, "FA Connecting Bridge" },
    { 0x6b0f0736, "Fatman fell with a sickening thud. \nSnake gazed down at the disgustingly \nfat body. Fatman's chest was moving \nfaintly up and down. He'd live.\n|\nSnake called Campbell by Codec and \ntold him what had happened. Campbell \nreported that the President was safe in \nthe custody of the Coast Guard. \nMoreover, the SEALs had entered the \nfacility while Snake was fighting \nFatman and were in the process of \ncleaning up the last remaining terrorists.\n|\n\"Why didn't you kill him?\"\nSnake turned around to see Ames \nstanding behind him.\n\"Why? I should be asking you the \nsame question. Why did you let the \nPresident go?\n\"What?\"\n\"Fatman told me everything. You were \nsupposed to be guarding the President.\"\nAmes laughed.\n\"Fatman? You buy what Fatman said?\"\nSnake did not move his eyes.\n\"Yeah, I do. Were you and Fatman a \nteam?\"\n\"Never.\"\n\"Is that so?\"\nSnake advanced toward Ames. Ames' \nhand reached for his chest.\n\"Wait a minute! You don't know what \nyou're talking about! He'd never do \nanything like that!\" Jennifer suddenly \nappeared. The SEALs must have \nrescued her.\n\"How can you be so sure?\" answered \nSnake, keeping his eyes fixed on Ames.\n\"Because Richard was the one who \nhelped the President escape!\"\n\"What are you talking about?\"\nSomething didn't add up. Fatman didn't \nseem to be lying. But then why had \nAmes cooperated with Snake? \nAnd now Jennifer was saying that Ames \nhad helped the President escape. Even \nif Fatman was lying and Jennifer was \ntelling the truth, why would Ames \nneglect to mention that the President \nwas safe?\nJennifer continued as Snake pondered \nthis riddle. \"Richard is one of our best \nagents. And besides, there's no way \nhe'd be involved in a terrorist bombing \nplot in the first place. After all, his \nsister -- \"\n\"Jennifer!\" Ames interrupted. She \nignored him.\n\"Richard lost his younger sister in a \nterrorist bombing attack. Three years \nago...\"\nA chill ran down Snake's spine. The \nwords came together in a rush. Three \nyears ago... terrorist bombing... sister... \nchurch... Fatman. Snake suddenly \nrecalled Ames' strange behavior. The \nway he spat as he said that Fatman \nhad been acquitted. His passionate \nconviction that the jury did not bring \njustice to Fatman. The look in his eyes \nas he gave Snake his gun and said, \n\"Promise me you'll kill him...\"  \nA hypothesis was starting to take shape.\nSnake looked at Ames. He had no \nfacial expression. No panic. No fear. \nNot even anger. The hypothesis had \nnow become a conviction.\n\"Ames, you didn't tell me that you'd \nalready rescued the President because \nyou thought I'd be less likely to kill \nFatman for you, right?\"\nAmes didn't respond. Snake continued.\n\"Did you call us instead of the police \nbecause I'd be more likely to kill \nFatman instead of capturing him alive?\"\n\"That's enough!\" yelled Jennifer. \nAmes' hand inched closer to Snake's \nchest.\n\"You planned all along to have me kill \nFatman, didn't you? And all this is \njust -- \"\nAmes suddenly pulled a pistol from his \nbreast pocket. Snake reacted \nimmediately, diving into the shadow of \na nearby crate. But Ames' gun was \npointed straight at Fatman's prostrate \nbody. He squeezed the trigger.\n\"Stop!\"\nJennifer forced Ames' hand aside, \nknocking the pistol from his grasp. \nIt arced through the air and fell with a \nclatter to the ground. Ames made a \ndesperate dive for it, but Snake's \ntrained reflexes were quicker. He \nreached out and grabbed the gun \nbefore Ames could get ahold of it. \nLying sprawled on the ground, Ames \nlooked up at Snake, then finally lowered \nhis shoulders and relaxed.\nJennifer staggered over to Ames.\n\"Tell me it isn't so...\"\nAmes slowly shook his head.\n\"So... it's true, then?\"\nAmes sat down wearily and stared up \nat the sky.\n\"Yes...\"\nHe told his story.\n\nAfter his sister was killed -- and Fatman \ngot off scot-free -- Ames had begun to \nplot his revenge. He approached an \nextremist group with a scheme to \nkidnap the President. Then he hired \nFatman to carry out the job. It had cost \nhim a small fortune just to hire a secret \nagent to track Fatman down and \nconvince him to join the conspiracy. \nAfter that, it was just as Snake had \ndescribed. Ames' plan was to lure \nFatman to the Big Shell and then have \nSnake kill him... all for the sake of his \ndear, departed sister. Jennifer crouched\ndown in front of Ames. \"Richard... \nwhy didn't you tell me...\"\n\"It's a little too heavy for pillow talk, \nwouldn't you agree?\"\nJennifer slapped his the face. Tears \nstreamed down her cheeks. Ames \nlooked away.\n\"I never meant for you to get involved. \nThere weren't supposed to be this \nmany hostages. Just the President.\"\n\"No, you don't get it.\"\nJennifer took Ames's hand in her own \nand clasped it tightly. She stared \nstraight into his eyes with a pleading \ngaze. But he returned no answer. \n\"What you did is a crime.\"\nDrooping her head in silence, Jennifer \nlooked as if she were being a bad \nloser. Ames answered.\n\"I know that. But Fatman must be \npunished.\"\n\"Nothing Fatman did could possibly \njustify what you've done!\"\n\"I know I can't bring my sister back \nfrom the dead. But put yourself in my \nposition for a minute. Do you really \nthink you could forgive him? He killed \nmy sister, for God's sake! And he \nnever spent a single day in prison for it. \nMy sister never had a chance to live \nout her life. And yet he sits there \ndrinking champagne, fooling around \nwith beautiful women, and blowing \npeople to bits. Doesn't that strike you \nas horribly wrong?\"\n\"Perhaps... but that doesn't make you \nany less of a criminal.\" \n\"Sorry, but I just don't see it the same \nway as you.\"\nAmes reached out with his finger and \nwiped away Jennifer's tears.\n\"So what are you going to do with me \nnow?\"\n\"I'm going to turn you in to the police.\"\n\"I'll probably get life in prison.\"\n\"Yeah, I guess so.\"\nAmes sighed. \"Before we go, there's \none last favor I need to ask of you.\"\nJennifer looked up at him. \"What?\"\n\"Please, let me kill the bastard.\"\nJennifer shook her head sadly. \"I can't \nlet you do that.\"\n\"I see... then I guess I'll just have to \nfind some other way to do it.\"\nAmes turned away and muttered to \nhimself.\n\"Lord knows I'll have plenty of time to \nthink.\"#C0\n" },
    { 0x72e0abd6, "Gap Between Parallel Universes" },
    { 0xcbc808d7, "GH Connecting Bridge" },
    { 0x019507bb, "He was in. Snake's underwater \ninfiltration of Big Shell was complete. \nAs he entered the elevator in the lower \npart of Strut A, he paused to reflect on \nhis briefing with Campbell.\nSix hours earlier, Big Shell had been \ntaken over by a group of heavily armed \nextremists. The terrorists had kidnapped \nthe President, who was visiting Big \nShell on an inspection tour, and taken \nhim hostage along with several \nemployees of the facility. Now they were \nusing the hostages to blackmail the \ngovernment. The terrorists' demands \nwere a joke: a list of 20-odd trivialities, \nplus $30 billion in cash.\n\"Your mission objectives are as follows,\" \nCampbell had said. \"First, infiltrate the \nocean cleanup facility known as 'Big \nShell' and rescue the President and the \nother hostages. Second, neutralize the \nterrorists. You are authorized to use \nany means necessary to complete this \nmission. Your contact will be able to fill \nyou in with more details later.\"\n\"Contact?\"\n\"One of the President's Secret Service \nagents, a man by the name of Richard \nAmes, seems to have eluded the \nterrorists.\"\nThe name triggered something in \nSnake's brain. He'd heard it before.\n\"Ames? Isn't he Nastasha's husband?\"\n\"Ex-husband,\" Campbell corrected him. \n\"He's also the one who requested our \nhelp in the first place. But whatever \nmeans he was using to communicate \nwith us has apparently been destroyed, \nand we can't get a hold of him anymore. \nYou've got to get in touch with Ames \ndirectly and find out anything you can \nabout the situation inside. His last \ntransmission came from Strut F, and he \nshould still be hiding somewhere in that \narea. That's where you should head first.\"\n|\nThe elevator began its ascent. As Snake \ntook off his diving equipment, he went \nover Campbell's last directive one more \ntime in his head. Ames was supposedly \nhiding from these terrorists. Even if \nSnake managed to get close to him, \nAmes wouldn't come out of hiding if \nSnake were being followed. Once he \narrived at Strut F and started looking \nfor Ames, he'd have to be extra careful \nto avoid being detected by the enemy. \nStrut F was located northeast of Strut A.#C0\n" },
    { 0x60b5f8ae, "HI Connecting Bridge" },
    { 0xc8c181f6, "homme allait lui r\303\251pondre. Baker avoua avec\ndouleur qu'il avait fourni cette information aux\nterroristes. Il avait le bras cass\303\251 et celui-ci\npendait, sans vie, le long de son corps :\nprobablement l'\305\223uvre de Ocelot.\n\343\200\200Selon les informations recueillies par Naomi\nHunter, Revolver Ocelot avait \305\223uvr\303\251 comme\nconsultant et dirig\303\251 des interrogatoires sp\303\251ciaux\ndans les goulags de l'ex-URSS, \303\240 l'\303\251poque o\303\271 il\nfaisait partie du Spetsnaz. En bref, il \303\251tait pass\303\251\n" },
    { 0x881494ce, "I have defused the explosives. The \nhostages are all safe now. The other \nhalf of the mission is suppressing the \nterrorist. Fatman must be on the heliport.\n" },
    { 0xe6649578, "i metodi che aveva usato per convincerlo.\n\343\200\200A detta delle informazioni in possesso di\nNaomi Hunter, ai tempi di Spetsnaz Revolver\nOcelot aveva lavorato come Consulente speciale\nper gli interrogatori nei gulag sovietici.\nInsomma, era un esperto nelle tecniche di\ntortura. Era impensabile che il direttore di\nuna societ\303\240 di tecnologia bellica, un civile\nnon addestrato, avrebbe potuto resistere alle\ntecniche di coercizione sviluppate nelle orrende\n" },
    { 0xa49e9df7, "I reached the Strut E ladder and \nclimbed down about 40 meters. When I \nfinally made it to the oil fence, I heard \nthe sound of gunfire from above. It \nwas coming from the Shell 1-2 \nconnecting bridge. I could see the \nSEALs fighting Vamp. But it wasn't a \nnormal fight. It was a one-sided \nmassacre. Vamp ran. The SEALs fired. \nBut the bullets didn't hit him. Vamp \nthrew knives. Some of the men went \ndown. Other SEALs fought back. Vamp \nwove and dodged. He slit the throats \nof the SEAL men, slicing their \nstomachs open and stabbing deep into \ntheir hearts. The SEALs went down in \na split second. I saw the last man \nscreaming at the totally unhurt Vamp. \nSuddenly, the connecting bridge \nexploded. It must have been wired up \nwith bombs beforehand. A series of \nexplosions enveloped the connecting \nbridge in flames, and it began to \ncollapse with a roar. Debris rained on \nthe surface of the sea. I made a run \nacross the oil fence. A huge column of \nwater rose behind my back. Waves \nchurned, throwing the oil fence up with \nevery undulation. I struggled along and \nfinally made it to the lower part of \nStrut L. As I turned around, my eyes \nmet Vamp's. He had tumbled from the \nsky and landed on the Shell 2 side. \nVamp sneered, licked a knife with his \nlong tongue and hurled the knife at me. \nAnd then he disappeared into the \ninterior of Strut G.\nI climbed the ladder and made it into \nStrut L of Shell 2. Vamp had already \npenetrated Shell 2. O'Brien was in \ndanger. I ran for the Shell 2 Core.#C0\n" },
    { 0xe19d4c33, "I was greeted at Strut B by the sight of \na total massacre. The bodies of dead \nSEALs littered the entire area. \nEverything had been torn apart and \nsplattered with blood. I heard a moan \ncoming from a body in a pool of blood.\n\"Hold on!\"\nI ran over and lifted the soldier in my \narms. He was still breathing, but his \nwound was deep, reaching his internal \norgans. It was clear that he wouldn't \nmake it. He looked at me with \nunfocused eyes.\n\"It was Vamp.... He called it a \n'lynching'.... revenge for those who \nbackstabbed Jackson. He called... the \nbribery charge... a sham...\"\n\"You'd better not talk now,\" I said, \ntrying to quiet him.\nBut he kept on talking.\n\"Vamp's going after Captain O'Brien... \nbut that's nonsense. There's no way \nthe Captain's involved... He's just... \nnot like that.\"\nNaval Captain O'Brien, multiply \ndecorated in the Persian Gulf, Europe, \nand Africa, was commander in chief for \nthe entire exercise. The mass media \npraised the Captain as a competent \nand noble-minded military man who is \nwell-liked by his fellow soldiers.\nHe grabbed my chest. His grip was \nsurprisingly strong for a man on his \ndeathbed. \n\"The Captain is... on the first floor of \nthe Shell 2 Core.... Save the Captain! \nPlease....\"\nHis body lost its strength. I closed his \neyes. That was all I could do for him.\nI contacted Dolph, telling him what I'd \njust heard.\n\"I see....\" mumbled Dolph.\nHis voice didn't sound surprised at all. \nIn fact, he sounded like he'd just \nconfirmed some bad news. \n\"Did you know about this?\" I asked.\nHe became quiet, answering hesitantly.\n\"There've been a lot of cases involving \nmurdered government employees and \nservicemen lately. Rumor has it Vamp \nis behind it all.\"\n\"Is Jackson really innocent?\"\n\"I don't know for sure, but perhaps. He \nwas a real proud soldier. He was not \nthe corrupt kind.\"\n\"You think O'Brien backstabbed \nJackson?\"\n\"No, I don't think so. I know his \nreputation....\"\n\"How can you be so sure?\"\n\"It was O'Brien who planned this \ntraining. If he was involved in Jackson's\ncase, there's no way he would involve \nVamp.\"\n\"That's only if O'Brien knew of the \nrumors of the lynching, right?\"\n\"He knew.\"\n\"Are you sure?\"\n\"Oh yeah. I heard of the rumors from \nO'Brien.\"\n\"...You and O'Brien talked about the \nrumors?\"\nI was a bit surprised. O'Brien and \nDolph... political Navy captain and \nhead of the Marines. They didn't seem \nlike a likely pair of friends. \nDolph answered as if he knew of my \ndoubts.\n\"O'Brien and I didn't always get along \nbecause of our positions. I don't think \nhe liked me either. But I don't think he \nhated me so much that he wouldn't give \nme advice that would save my life.\"\n\"What do you mean?\"\n\"O'Brien came to warn me -- that I was \nVamp's next target.\"\nI then knew the true reason Dolph had \nparticipated in this exercise.\n\"So you tried to rid Dead Cell of its \ninfamous rumors by revealing yourself \nto Vamp....\"\n\"Correct. If nothing happened to me \nduring the exercise, I could prove that \nthe rumors were false.\"\n\"...You do trust Vamp.\"\nDolph scoffed.\n\"I wouldn't go that far. If I trusted him \ncompletely, I wouldn't have called you....\nI'm a coward.\"\nI said to him that trusting something or \nsomeone isn't that easy. Dolph did not \nrespond. I then asked him. \"But then \nit's not you but O'Brien that Vamp's \nafter?\"\n\"Yeah.... Either O'Brien's information \nwas wrong or Vamp's wrong...\"\nIt seemed Dolph was confused too.\n\"The only thing that's certain is that \nVamp is trying to kill O'Brien.... Snake, \nyou've got to stop Vamp.\"\n\"Got it.\"\nI considered the structure of Big Shell \nagain. I'd need to use the Shell 1-2 \nconnecting bridge from Strut D to get \nto Shell 2, where Vamp should be \nheading. That meant I'd have to head \nfor Strut D first.#C0\n" },
    { 0x6cf07e3b, "I'm smack in the middle of the \nbattleground on a mission. The roar of \ncannons and gunfire punctuate the \ndarkness. Conditions are unbelievably \nharsh this time around. No one on \neither side is without injury, and the \nbodies of the dead lie sprawled \neverywhere. It's hell on Earth. \nI instinctively duck, feeling a bullet cut \nthrough the air inches away. Just then, \nthe Codec rings. It's Rose. \n\"Do you want to.... Jack? What \nhappened to the mission?\"\n\"I'm in the middle of it!\"\n\"Oh, that's right.\"\n\"What is it?\"\nRose hesitated for a moment before \nshe spoke.\n\"...Jack, I need to talk.\"\n\"Talk?\"\n\"Yes, it's very important. And we'll talk \nabout it, tomorrow.\"\nWhy is she telling me now about \nsomething she wants to discuss \ntomorrow? Confused, I put the \nquestion to her:\n\"Why not now?\"\nRose eagerly said:\n\"All right, I'll tell you. Jack... I'm... \nI'm carrying...\"\nA heavy silence filled the air. Finally, \nRose seemed to make up her mind.\n\"...I think I'm gaining weight! I'm \ngetting love handles. What should I \ndo?\"\n\"What should you...? I didn't notice at \nall!\"\n\"Really? But my old skirt is getting \nkind of tight on me....\"\n\"If it really bothers you that much, why \ndon't you start working out? I'll even \njoin you if you like.\"\n\"That's great! How about jogging? \nOh, before I forget, your mission this \ntime is to hold up all enemies and get \nto the goal. Good luck.\"#C0\n" },
    { 0x7f85b42f, "If you prefer to stay in one piece, |you'll have to disable my bombs." },
    { 0xa6844541, "IJ Connecting Bridge" },
    { 0x30b10f52, "J'atteignis l'\303\251chelle de l'Etai E et descendis\nenviron 40 m\303\250tres. Une fois sur la barri\303\250re\nflottante, j'entendis des coups de feu venant\nd'en haut. Ils venaient de la passerelle de\nconnexion 1-2. Je vis des SEAL lutter\ncontre Vamp. Mais ce n'\303\251tait pas une\nbataille ordinaire. C'\303\251tait un massacre.\nVamp courait. Les SEAL tiraient. Mais les\nballes ne le touchaient pas. Vamp lan\303\247ait\ndes couteaux. Des hommes tombaient.\nD'autres luttaient. Vamp esquivait tous les\ncoups. Il tranchait les gorges, ouvrait les\nventres et perforait les c\305\223urs. Les SEAL\ntomb\303\250rent en quelques secondes. Je vis le\ndernier homme hurler face \303\240 Vamp qui\nn'avait m\303\252me pas une \303\251gratignure. Soudain,\nla passerelle de connexion explosa. Des\nbombes avaient d\303\273 y \303\252tre pos\303\251es plus t\303\264t.\nUne s\303\251rie d'explosions mit la passerelle en\nfeu et elle commen\303\247a \303\240 s'\303\251crouler dans un\nrugissement. Il se mit \303\240 pleuvoir des d\303\251bris\nsur l'eau. Je traversai la barri\303\250re flottante en\ncourant. Une \303\251norme colonne d'eau s'\303\251leva\nderri\303\250re moi. La mer \303\251tait d\303\251cha\303\256n\303\251e et la\nbarri\303\250re bougeait dans tous les sens. Je\nparvins avec peine au bas de l'Etai L. Je me\nretournai et mon regard croisa celui de\nVamp, qui \303\251tait tomb\303\251 du ciel du c\303\264t\303\251 de la\nShell 2. Vamp ricana, l\303\251cha un couteau de\nsa longue langue et le lan\303\247a contre moi.\nPuis il disparut \303\240 l'int\303\251rieur de l'Etai G.\nJe grimpai \303\240 l'\303\251chelle et entrai dans l'Etai L\nde la Shell 2. Vamp y \303\251tait d\303\251j\303\240. O'Brien\n\303\251tait en danger. Je courus vers le noyau de\nla Shell 2.#C0\n" },
    { 0x9bd49a95, "J'\303\251tais \303\240 peine arriv\303\251 \303\240 l'Etai D que Dolph\nme contacta. Il avait pu parler \303\240 O'Brien.\nO'Brien avait bloqu\303\251 la porte menant de\nl'Etai D \303\240 la passerelle de connexion 1-2\npour emp\303\252cher Vamp de passer. Il avait\n\303\251galement r\303\251organis\303\251 les SEAL qui restaient\npour d\303\251fendre l'Etai G. Dolph dit :\n\n\"Snake, rejoins les SEAL. Vamp est\nl'ennemi. Ces hommes sont peut-\303\252tre des\nSEAL, mais je doute qu'ils puissent\nl'arr\303\252ter.\" \n\"Mais comment passer \303\240 la Shell 2 ? La\npasserelle de connexion 1-2 est ferm\303\251e.\"\n\n\"Il y a un autre chemin. Tu devrais pouvoir\natteindre l'Etai L de la Shell 2 en traversant\nla barri\303\250re flottante, au bas de la Big Shell.\nPrends l'\303\251chelle au niveau B1 de l'Etai E\npour descendre.\" \nVamp devait d\303\251j\303\240 \303\252tre parti pour la Shell 2.\nJe devais faire vite.#C0\n" },
    { 0x988d2534, "Jennifer looked as if she were about to \nsay something. Snake swiftly reached \nout and covered her mouth with his \nhand.\n\"Keep your voice down. They'll hear us.\"\nSnake removed her blindfold, keeping \nan eye out for approaching guards. \nUpon seeing Snake's face, Jennifer's \nclear brown eyes glowed with joy. \nBut now was not the time to grant her \nwish. Snake said to her,\n\"I don't mean to disappoint you, but my \nfirst priority is to rescue the President. \nYou'll just have to be patient until I get \nhim to a safe place.\"\nJennifer's shoulders sagged.\n\"First, though, I need you to tell me \neverything you know. If you understand \nme, nod your head.\"\nJennifer nodded. Snake lifted his hand \nfrom her mouth.\n\"What's going on here?\"\n\"Well, when they tied me up and \ndumped me here on the floor, I thought \nI'd never get out alive. Then this guy \ncame to rescue me. But he turned out \nto be a cold-blooded jerk, and now I'm \nreally mad.\"\nSnake was at a loss for words. \"I... \nthat's not...\"\n\"Just kidding. You're looking for the \nPresident, right? He's already gone,\" \nsaid Jennifer, almost casually. She \ncontinued while enjoying Snake's \npuzzled look,\n\"He escaped through the bottom of one \nof the struts. He should be on a boat \nback to Manhattan right about now. One \nof my fellow agents managed to sneak \nhim out as we were being attacked.\"\n\"Are you sure?\"\n\"Yes. Pretty good, aren't I?\" Jennifer \nbeamed with pride. \"Too bad we got \ncaught before we could see him off....\nSo, that's that. Maybe it's about time \nfor you to save me.\"\nIf the President had escaped safely, \nsaving the hostages was the new \nmission. Snake nodded.\n\"...OK. I've gotta take care of that guard \nfirst.\"\nAs Snake rose, Jennifer stopped him.\n\"That's not a good idea.\"\n\"Why not?\"\n\"They supposedly set a bomb in here. \nThey said they'd set it off if we tried to \nescape.\"\nSnake made a small grunt. This wasn't \ngoing to be that easy.\n\"Do you know where it is planted?\"\n\"As far as I've been able to tell, it \nshould be somewhere here in the Shell \n1 Core.\"\n\"All right. I'll get to that first.\"\nJust then, he overheard one of the \nterrorist guards talking to his comrades \non the radio. It sounded like they were \ngoing to tighten the security in the \nhostage room. He'd have to get out fast.\n\"The terrorists don't seem to have any \nintention of harming the hostages. For \nnow, at least, sit tight and pretend to be \ncaptured for a little while longer.\"\n\"Sure. I've got a Level 2 security card \nhidden in my jacket. Go ahead and take \nit.\"\nAs Snake felt through the jacket for the \ncard, Jennifer let out a squeal.\n\"Hey, watch your hands, mister! \n... that's it, just a little lower... there, \nthere it is.\"\nAs Snake retrieved the level 2 card, \nJennifer looked into his eyes.\n\"Hurry back, okay?\"\n\"Don't worry, I will.\"\nSnake put the blindfold back around \nJennifer's eyes, then made his way out \nof the assembly hall.#C0\n" },
    { 0xdecf88d3, "JK Connecting Bridge" },
    { 0xf19638de, "KL Connecting Bridge" },
    { 0x16df023d, "La navaja ten\303\255a la palabra \"Spetsnaz\"\nescrita por todas partes...\n" },
    { 0xdd2bd3fc, "La SOCOM dura pi\303\271 di 6.000 colpi,\n" },
    { 0x77675018, "La tua SOCOM \303\250 provvista di silenziatore.\n" },
    { 0x1153ec43, "Le mot Spetsnaz \303\251tait grav\303\251\nsur son couteau de scout...\n" },
    { 0xba250561, "LG Connecting Bridge" },
    { 0xdfae0f46, "Meryl and Gurlukovich were a team. Sure I was\nshocked, but I managed to escape the holds. \nThe enemies are right behind me. I must first \nget out of Deck 2 here. Maybe if I go back to \nthe engine room there is a way out.\n" },
    { 0xb6fb14ab, "Modalit\303\240 Arma Livello SOCOM 01\n" },
    { 0x49a4b150, "Mode Arme, Niveau SOCOM 01\n" },
    { 0xed8017da, "Modo Arma SOCOM, Nivel 01\n" },
    { 0x321332c5, "Navigational Deck, Wing" },
    { 0xdec86f73, "noch klarer erscheinen lassen.\n\n- Revolver-Ocelot\n\343\200\200Ein ehemaliger Angeh\303\266riger der Spetsnaz, der\nauch als \"Shalashaska\" bekannt war. Nach dem\nZusammenbruch der Sowjetunion fand er lukrative\nAuftr\303\244ge als S\303\266ldner in konfliktgebeutelten\nRegionen auf der ganzen Welt. Seine Aktivit\303\244ten\nresultierten schlie\303\237lich darin, da\303\237 ihn die\nRegierung der USA rekrutierte, und er kam zur\n" },
    { 0x0e172748, "noch klarer erscheinen lassen.\n\343\200\200\n- Revolver-Ocelot\n\343\200\200Ein ehemaliger Angeh\303\266riger der Spetsnaz, der\nauch als \"Shalashaska\" bekannt war. Nach dem\nZusammenbruch der Sowjetunion fand er lukrative\nAuftr\303\244ge als S\303\266ldner in konfliktgebeutelten\nRegionen auf der ganzen Welt. Seine Aktivit\303\244ten\nresultierten schlie\303\237lich darin, da\303\237 ihn die\nRegierung der USA rekrutierte, und er kam zur\n" },
    { 0x5e2c3635, "Olga Gurlukovich, Tochter von Oberst\nSergei Gurlukovich, ehem. GRU-Mitglied\nund fr\303\274herer Spetsnaz-Kommandeur.\n" },
    { 0x1da5dfe8, "Olga Gurlukovich... Figlia del Colonnello\nSergei Gurlukovich, ex GRU ed ex\ncomandante di Spetsnaz...\n" },
    { 0x8467a1f2, "Olga Gurlukovich... La fille du Colonel\nSergei Gurlukovich, ex-GRU et ancien\ncommandant des Spetsnaz...\n" },
    { 0x83f78313, "Perch\303\251 sei cos\303\254 in ritardo|con il rapporto?" },
    { 0xc233a791, "Philanthropy's Hideout" },
    { 0x99d58551, "plus grands tireurs de l'histoire. Ceux qui\nsemblent avoir \303\251t\303\251 le plus impliqu\303\251s dans la\nconspiration du gouvernement sont pr\303\251sent\303\251s\nci-dessous. Ces informations devraient jeter un\npeu de clart\303\251 sur la terrifiante v\303\251rit\303\251 de toute\ncette affaire.\n\n- Revolver Ocelot\n\343\200\200Ex-Spetsnaz \303\251galement connu sous le nom de\n'Shalashaska'. Apr\303\250s l'effondrement de l'Union\n" },
    { 0xa4dc3c69, "plus grands tireurs de l'histoire. Ceux qui\nsemblent avoir \303\251t\303\251 le plus impliqu\303\251s dans la\nconspiration du gouvernement sont pr\303\251sent\303\251s\nci-dessous. Ces informations devraient jeter un\npeu de clart\303\251 sur la terrifiante v\303\251rit\303\251 de toute\ncette affaire.\n\343\200\200\n- Revolver Ocelot\n\343\200\200Ex-Spetsnaz \303\251galement connu sous le nom de\n'Shalashaska'. Apr\303\250s l'effondrement de l'Union\n" },
    { 0x394111c1, "Puedes colocar el silenciador en 10\ndiferentes lugares de tu SOCOM.\n" },
    { 0x170969c7, "Puoi fissare un silenziatore alla SOCOM per\nnon farti sentire. Cerca di trovarne uno.\n" },
    { 0x10790ea9, "Quella pistola che hai si chiama SOCOM.\n" },
    { 0x4d191b4b, "receiving news of Snake's arrival.\n\343\200\200 According to Naomi Hunter, the former\ndirector of FOXHOUND's genetic manipulation\nprogram, Revolver Ocelot is a former\nSpetsnaz. He moved into OMON (Otryad\nMilitsii Osobogo Naznacheniya, the Interior\nMinistry riot squad, AKA Black Berets) and\nthe SVR (the Russian Foreign Intelligence\nService) -- a successor to the KGB's First\nChief Directorate -- after the collapse of the\n" },
    { 0x06f94ca5, "Reinforcements are on their|way to B2 of the Shell 1 Core. " },
    { 0xaa902df0, "remote place, north of Alaska's Fox Islands.\nThough it was hardly public knowledge, the\nisland was home to a nuclear weapons\ndisposal facility.\n\343\200\200\n\343\200\200\343\200\200According to the terms of the START2\n(Strategic Arms Reduction Treaty), the total\nnumber of tactical nuclear warheads owned by\nthe U.S. and Russia was reduced to some-\nwhere between 3000 and 3500 in the later\n" },
    { 0x1f5388de, "Revolver Ocelot\nUn antiguo Spetsnaz, tambi\303\251n conocido como\n\"Shalashaska\". Tras el derrumbamiento de la\nUni\303\263n Sovi\303\251tica, trabaj\303\263 como mercenario a\nsueldo en diferentes conflictos regionales del\nmundo. Tras probar sus habilidades, fue\nreclutado por el gobierno de los Estados Unidos,\ny finalmente pas\303\263 a formar parte de FOXHOUND.\nTal y como su nombre en clave indica, es un\n" },
    { 0x4f586316, "sconcertato Snake.\n\343\200\200\"Tu non potresti capire neanche questo. Uno\nche ha ucciso suo padre non potrebbe! Sei\nriuscito persino a privarmi della mia vendetta.\nMa io far\303\262 avverare il sogno che nostro padre\nnon ha mai realizzato. Ecco come lo uccider\303\262 -\nsuperandolo nettamente.\" Liquid ha terminato il\nproclama balzando nell'abitacolo di Metal Gear.\nSnake ha fatto fuoco con la SOCOM, ma i\nproiettili sono rimbalzati sulla corazza di Metal\n" },
    { 0xafc5c65a, "Seg\303\272n Naomi Hunter, el antiguo director del\nprograma de manipulaci\303\263n gen\303\251tica de\nFOXHOUND, Revolver Ocelot era un antiguo\nSpetsnaz. De ah\303\255 pas\303\263 a OMON (Otryad Militsii\nOsobogo Naznacheniya, la brigada antidisturbios\ndel Ministerio del Interior, tambi\303\251n conocido como\nBoinas Negras) y el SVR (Servicio de Inteligencia\nExtranjera Sovi\303\251tica), sucesor del Primer Jefe de\nla KGB, que tras la ca\303\255da de la Uni\303\263n Sovi\303\251tica\nno fue capaz de adaptarse al nuevo r\303\251gimen y\n" },
    { 0x86be7f94, "Seg\303\272n Naomi Hunter, Revolver Ocelot hab\303\255a\ntrabajado para el Equipo Especial de\nInterrogatorios, en los gulags sovi\303\251ticos durante\nel tiempo que estuvo con Spetsnaz. En otras\npalabras, era un experto en torturas. No era\nposible que el ejecutivo, un civil sin\nentrenamiento militar, pudiera aguantar las\nt\303\251cnias de coacci\303\263n y tortura aprendidas en las\nceldas de Lubyanka. Ahora ya sab\303\255amos que los\nterroristas sab\303\255an los c\303\263digos para el\n" },
    { 0x085f84af, "Shell 1-2 Connecting Bridge" },
    { 0x2ff82416, "Shell 1-2 Connecting Bridge" },
    { 0x2adee1d5, "Shell 1-2 Connecting Bridge - LG Connecting Bridge" },
    { 0xdde44d03, "Snake accessed the node. He entered \nthe password that Emma had given him. \nAfter a moment, the monitor displayed \na message saying that the lock on the \ndoor to the Shell 1-2 connecting bridge \nhad been released. Snake turned to \nhead back to Strut C, he heard a voice \nemanating from an unseen speaker. It \nsounded like some kind of building-wide \nannouncement.\n\"This is a message for the 'ghost of Big\nShell'. You are hereby ordered to \nremove the lock on the system using the\ncontrol key you got from the girl at \nonce. You have fifteen minutes to \ncomply. If you fail to do so, we will kill \nthe girl. I repeat...\"\nThe voice repeated its warning. A chill \nran up his spine. The girl they were \ntalking about had to be Emma. Had \nthey really gotten their hands on her? \nAnd what was this control key they \nwere talking about? Snake called Otacon \nvia the Codec.\n\"They got Emma?!\" Otacon exploded. \n\"What did you...\"\n\"Just calm down for a minute. I'm trying \nto think of a way to save her.\"\nThe calm returned to Otacon's voice. \n\"All right.\"\n\"This control key that they're talking \nabout, could it be this card that Emma \ngave me?\"\n\"No, that's just a security card. It's \nonly good for opening doors. The \ncontrol key is just what it sounds like -- \nan actual key that only Big Shell's \nsystem administrator would have access \nto. It acts as a verification device. \nBasically, you can't make any changes \nto Big Shell's computer system without \nit. The terrorists need it to release the \nlock on the system....\"\n\"What if they used a password like I \njust did?\"\n\"That wouldn't work. Passwords are \nonly good up to security level 3. Beyond \nthat, you'd need the control key to gain \naccess....\"\n\"Sounds like you've been doing your \nhomework.\"\n\"Snake, are you sure Emma didn't give \nyou anything else?\"\n\"Yeah. What makes them think I have \nit, anyway?\"\n\"...Maybe Emma lied to them.\"\n\"Or maybe she gave it to the real 'ghost \nof Big Shell.'\"\n\"That's not possible,\" said Otacon \nquickly.\n\"And how can you be so sure?\" \ncountered Snake.\n\"Hey, you said yourself that there's no \nsuch thing, didn't you? We have to \nhurry and save Emma!\"\n\"You're right. But we don't even know \nwhere they've taken her....\"\nOver the Codec, Snake heard the sound \nof Otacon smacking one hand against \nthe other.\n\"That's it! Why didn't I think of it \nbefore?\"\n\"What?\"\n\"Emma said she had her own card with \nher, right? Security cards in Big Shell \nwork on an RFID system -- that's radio \nfrequency identification. Each RFID \ncard is embedded with an IC chip that \nsends data via a radio signal. The card \nreaders can use these signals to detect \nany IDs within range. All data in Big \nShell is managed by an integrated \nmanagement system. And that system \nis accessible with a level 3 password!\" \nOtacon was speaking rapidly now.\n\"...Meaning?\" said Snake.\n\"Meaning that you can use the node to \nfind out which strut Emma is in! Snake, \ntry accessing the node again using that \npassword.\"\nSnake did as Otacon said. After a \nmoment, the screen displayed a blinking \nlight in Strut F.\n\"That's her. She's in Strut F. Snake, \nyou've got to hurry!\"#C0\n" },
    { 0xf8da59d2, "Snake addressed the man.\n\"You must be Ames.\"\nAmes nodded. \"And you must be Solid \nSnake.\"\nLearning to read Snake's shrugs, Ames \ntook this as a \"yes\".\n\"I knew they'd send you. After all, \nI asked for the best man they had. That \nwas quite an impressive job you pulled \non that tanker a few years ago.\"\n\"You mean slaughtering those terrorists \nto the last man?\" He felt the painful \nmemories resurfacing in his mind -- \nscores of fresh, young soldiers lying \ndead in heaps on the deck, the acrid \nsmell of blood permeating every corner \nof the ship....\n\"But you managed to get all the \nhostages out safely, didn't you?\" \nasked Ames.\nSnake had heard these words countless \ntimes. They never helped ease the \npain. They never would. Snake did not \nfeel like explaining that.\n\"Where is the President?\" he said, \ngetting to the point.\nAmes looked away.\n\"I...don't know.\"\n\"What about the other hostages?\"\n\"...I'm not sure.\"\n\"Are you telling me you don't know \nanything?\"\n\"I've seen the leader of the terrorists.\"\n\"Who? Who is it?\"\n\"Fatman.\"\nSnake searched his memory. It didn't \ntake long for him to trace the name.\nFatman. A disgraced former Navy SEAL \nturned mad bomber. The self-styled \n\"Emperor of Explosives.\" A man with no \nphilosophy or creed. A hardened criminal \nwho'd sell himself to anyone for the \nright price. But hadn't he been arrested \nthree years earlier after that \nchurch-bombing incident?\n\"He was acquitted,\" Ames spat in \ndisgust, as if anticipating Snake's \nquestion.\n\"Even though more than thirty people \ndied in the blast?\"\n\"The jury found him not guilty. They said \nthere was still a 'reasonable doubt' as \nto whether he had done it or not. \nFatman's got a lot of rich and powerful \nfriends who can afford to hire the best \ndefense attorneys in the business. He \nbought his freedom -- with the same \nblood money he made from killing those \ninnocent people!\"\nAmes' mouth twitched. Maybe he was \ngoing to grin.\n\"If he was judged correctly, this would \nnot have happened.... In other words, \nthe jury's mistake is about to kill us, \nand their President.\"\nAmes turned around toward Snake.\n\"Snake, rescue the President. I know \nFatman was at the heliport. He's \nprobably still giving orders there now.\"\n\"The heliport... On the roof of Strut E, \nright?\"\nAs Snake nodded and got up to leave, \nAmes stopped him.\n\"Don't waste your time trying to take \nhim alive. He's got too many tricks up \nhis sleeve. And the President's life is \nstill in danger.\"\nAmes took out his USP and looked \nstraight into Snake's eyes.\n\"Promise me you'll kill him.\"\nHe handed the USP to Snake. Then he \nproduced a few more objects from his \njacket.\n\"Here, you'd better take these along \nwith you, too. I picked them up on my \nway here. They should come in handy \nwhen you're taking care of Fatman.\"\nCoolant spray, the bomb-detecting \nSensor A, and a Level 1 security card. \nSnake took the items and added them to \nhis equipment.\n\"All right. Leave the rest to me.\"\n\"I'm counting on you. I think I'll just \nstay here for a while.\"\n\"Sounds like a good plan to me.\"\nSnake left the room. He heard a \"click\" \nas the door locked behind him. True to \nhis word, Ames was planning to hole up \nin that room. \nThe Codec sounded. It was Campbell.\n\"Snake, we've found out where the \nhostages are being held.\"\n\"How?\"\n\"By e-mail. Someone in there is using \ntheir cell phone to send messages to \nthe government -- a Secret Service \nagent named Jennifer.\"\n\"Where are the hostages?\"\n\"She says they're in the Shell 1 Core, \non level B1.\"\n\"I found out where the terrorist leader \nis, too. What's my priority here: \nthe leader or the hostages?\"\n\"I'll leave that up to you, Snake. \nGood luck.\"#C0\n" },
    { 0xa348cf40, "Snake cerr\303\263 la puerta herm\303\251tica y gir\303\263 la\nmanilla para bloquear la puerta. Esto le\nhar\303\255a ganar algo de tiempo, pens\303\263. Entre\ntanto, quer\303\255a comprobar algo. Llam\303\263 a\nCampbell por el Codec y le cont\303\263 lo que\nhab\303\255a sucedido.\n\"Meryl y Gurlukovich...\" dijo Campbell, le\ntemblaba la voz. \"Meryl dijo algo sobre \"la\n\303\272ltima voluntad de su padre\". \302\277Qu\303\251\ndemonios estaba haciendo tu hermano en\neste pa\303\255s?\"\nCampbell se lo explic\303\263.\nDurante la Guerra Fr\303\255a, cuando el pa\303\255s\ntodav\303\255a estaba bajo el control de un dictador\npro americano, el ej\303\251rcito americano estaba\nllevando a cabo una operaci\303\263n top-secret en\nEldera. Matt Campbell era un miembro de\nese proyecto. Cuando se acab\303\263 la Guerra\nFr\303\255a, tambi\303\251n se dio por finalizado dicho\nproyecto, y por lo tanto los americanos se\nprepararon para abandonar el pa\303\255s. Matt, sin\nembargo, desapareci\303\263 en combate. Poco\ndespu\303\251s, el r\303\251gimen pro americano, sin el\napoyo que Am\303\251rica le ofrec\303\255a, se tuvo que\nenfrentar a levantamientos de grupos\nminoritarios del pa\303\255s. En Eldera estall\303\263 una\nguerra civil. Poco despu\303\251s se encontr\303\263 el\ncad\303\241ver de Matt. Spetsnaz estaba detr\303\241s de\naquel levantamiento. Y el nombre de su\ncomandante era Sergei Gurlukovich. \"Hay\nquien dice que Matt estaba colaborando con\nGurlukovich. Pero yo no me lo creo.\"\nCampbell dejaba entrever que \303\251l cre\303\255a que\nfue precisamente Gurlukovich quien asesin\303\263\na Matt.\n\"\302\277Y por eso llevas tanto tiempo obsesionado\ncon Gurlukovich?\" pregunt\303\263 Snake. \"... S\303\255,\"\nadmiti\303\263 Campbell. Odiaba a Gurlukovich. El\nodio engendraba odio.\n\"\302\277T\303\272 tambi\303\251n estabas involucrado en ese\nproyecto?\"\n\"No. Hay una regla que lo impide. No\npodemos participar en operaciones en las\nque ya participen miembros de nuestra\nfamilia. Dicen que, al haber lazos de\nparentesco, la misi\303\263n se puede ver\nafectada.\"\n\"\302\277\303\211sa fue la \303\272nica raz\303\263n?\"\n\"... Bueno no, yo creo que Matt por aquel\nentonces quer\303\255a que hubiera cierta distancia\nentre nosotros.\" La voz de Campbell\ndenotaba cierto pesar. Pero no explic\303\263 las\nrazones que hab\303\255a tras las acciones de su\nhermano. \"El viejo Metal Gear que has visto\nes el resultado de aquel proyecto... Eso es\ntodo lo que s\303\251. Todo ese asunto es muy\nconfidencial. Incluso al d\303\255a de hoy, todos los\narchivos con informaci\303\263n sobre ese proyecto\nest\303\241n precintados.\"\nLe creo, pens\303\263 Snake. Despu\303\251s de todo es\nmuy normal intentar olvidar hechos o\nacontecimientos desagradables que\nocurrieron en el pasado. Y los gobiernos no\nson una excepci\303\263n. Pero hab\303\255a gente que se\naferraba a ese tipo de recuerdos. Hab\303\255a\ngente que nunca olvidaba su odio. Como\nGurlukovich y Campbell.\n\"Snake, no podemos permitir que\nGurlukovich y sus hombres escapen con el\nMetal Gear,\" dijo Campbell. \"Puede que\ntodav\303\255a est\303\251 cargado con cabezas nucleares\nactivas. Probablemente est\303\251n planeando\nprobar el Metal Gear en alg\303\272n lugar en el\nmar. Tienes que cambiar el rumbo del\nbarco. Dir\303\255gete hacia el puente.\"\nSnake no ten\303\255a ninguna objeci\303\263n.#C0\n" },
    { 0x84354404, "Snake ferma la porte \303\251tanche derri\303\250re lui et\ntourna la poign\303\251e pour la verrouiller. Cela\ndevrait lui laisser du temps, pensa-t-il. Il\navait quelque chose \303\240 v\303\251rifier. Il appela\nCampbell sur le Codec et raconta ce qui\ns'\303\251tait pass\303\251.\n\"Meryl et Gurlukovich...\" La voix de\nCampbell tremblait.\n\"Meryl a parl\303\251 de la \302\253 derni\303\250re volont\303\251 de\nson p\303\250re \302\273. Mais qu'est-ce que ton fr\303\250re\nfaisait dans ce pays, bon sang ?\"\nCampbell expliqua.\nPendant la Guerre froide, alors que le pays\n\303\251tait entre les mains d'un dictateur\npro-am\303\251ricain, l'arm\303\251e am\303\251ricaine avait\nmen\303\251 un projet top secret \303\240 Eldera. Matt\nCampbell y participait. Une fois la Guerre\nfroide termin\303\251e, le projet prit fin et les\nAm\303\251ricains se pr\303\251par\303\250rent \303\240 quitter le pays.\nMais Matt partit sans permission. Peu\napr\303\250s, ayant perdu son assise, le r\303\251gime\npro-am\303\251ricain dut faire face \303\240 des\nsoul\303\250vements arm\303\251s au sein des minorit\303\251s\ndu pays. Eldera \303\251tait d\303\251chir\303\251e par la guerre\ncivile. Le corps de Matt fut d\303\251couvert plus\ntard. Le Spetsnaz \303\251tait derri\303\250re\nl'insurrection. Et le nom de son\ncommandant \303\251tait Sergei Gurlukovich.\n\"Certains disent que Matt collaborait avec\nGurlukovich. Mais je ne le crois pas un\ninstant.\" Campbell laissait entendre qu'il\npensait que Gurlukovich avait tu\303\251 Matt.\n\"C'est pour cela que tu es obs\303\251d\303\251 par\nGurlukovich depuis ?\" demanda Snake.\n\"... Oui,\" admit Campbell. Il ha\303\257ssait\nGurlukovich. La haine nourrissait la haine.\n\"Toi aussi tu \303\251tais du projet ?\"\n\"Non. Le r\303\250glement l'interdit. Pas de\nmembres de la m\303\252me famille dans une\nop\303\251ration. Cela pourrait fausser notre\njugement.\"\n\"C'est tout ?\"\n\"... Je suppose que Matt voulait alors mettre\nune certaine distance entre nous.\" La voix\nde Campbell \303\251tait teint\303\251e de regret. Mais il\nn'offrit aucune explication quant aux actions\nde son fr\303\250re. \"Le vieux Metal Gear que tu\nas vu est le r\303\251sultat de ce projet... C'est\ntout ce que je sais. Toute l'affaire est\nhautement confidentielle. Les dossiers sont\nencore sous scell\303\251s.\"\nJe le crois, pensa Snake. Apr\303\250s tout, il est\nnaturel d'essayer d'oublier les horreurs du\npass\303\251. Les gouvernements ne font pas\nexception \303\240 la r\303\250gle. Mais certains\ns'accrochent \303\240 ce genre de souvenirs. Il y a\ndes gens qui n'oublient jamais leur haine.\nComme Gurlukovich et Campbell.\n\"Snake, nous ne pouvons pas laisser\nGurlukovich et ses hommes s'enfuir avec ce\nMetal Gear,\" dit Campbell. \"Il est peut-\303\252tre\nencore \303\251quip\303\251 de t\303\252tes nucl\303\251aires actives.\nIls pr\303\251voient s\303\273rement de faire des essais\nquelque part en mer. Tu dois changer le\nnavire de direction. Va sur la passerelle.\"\nSnake n'avait pas d'objection.#C0\n" },
    { 0xbaabfe4e, "Snake had found Emma.\n\"Emma!\"\nEmma looked up without rising from \nher crouched position. \"You're late!\" \nShe had a sullen look on her face, but \nshe didn't appear to be hurt.\n\"Sorry. It took me a while to find the \ncontrol key.\"\nEmma stuck out her tongue at Snake's \nsarcasm. \"Oh, that.... You have to admit \nit did buy us some time, didn't it?\"\nShe showed no sign of remorse. \nJust as Otacon had guessed, Emma \nhad indeed lied to the terrorists.\n\"What in the world made you do a thing \nlike that? If you'd made one false \nmove...\"\n\"Hey, I just didn't want them to get \naway with it, that's all. And... I believed \nin you.\"\n\"Believed what?\"\n\"That you'd come and rescue me, of \ncourse.\"\nSnake shook his head. \"That's not the \npoint. The point is, you shouldn't be \nputting yourself in so much danger....\"\n\"I know. But listen, you're never gonna \nbelieve this! I found out what the \nterrorists are looking for!\"\n\"What is it?\"\n\"Drugs!\"\nEmma related a story she'd heard from \none of the guards while she was being \nheld captive.\nThe armed group that had taken over \nBig Shell belonged to a Russian crime \nsyndicate. With the collaboration of a \nnumber of employees in the facility, the \nsyndicate was using Big Shell as a \nwaypoint in their drug smuggle \noperations. At night they'd sneak the \ndrugs from smuggling ships out at sea \ninto Big Shell using small boats. Then \nthey'd slip the drugs in with the \noutgoing cargo from Big Shell and get \nthem into the country that way. Security \nin the harbor had been getting tighter in \nrecent years, and this was a convenient \nand effective way of getting the goods \nthrough without being exposed.\nOne month earlier, they'd brought in a \n600 kg shipment of drugs with a street \nvalue of over $7 million. The following \nday, two of the employees who were \nworking with the syndicate were killed \nin two consecutive accidents. The \nsyndicate immediately sent one of their \nmen in to investigate, but he too was \nkilled in an accident shortly after \narriving.\nThe leaders of the syndicate decided \nthat these incidents were the work of a \nrival syndicate that had been competing \nwith them in the drug trade and that \nwere now trying to seize their shipment. \nThey'd sent in their troops in to recover\nthe drugs, leading to the current \nsituation. \n\"So the 'ghost of Big Shell' is...\"\nA mischievous grin spread across \nEmma's face. \"Exactly. It's not a ghost \nat all. It's just a plain old hitman who's \nbeen sent by the rival syndicate to \nsteal the drugs.\"\n\"...And you told them that I'm the \nhitman.\"\n\"Well... yeah, but what's done is done, \nright? No use arguing about it.\"\nSnake decided not to pursue the matter \nany further. He changed the subject. \n\"So where could they be hiding such a \nhuge amount of drugs in Big Shell?\"\n\"I have no clue. I didn't have any idea \nwhat was going on. I'm still having \ntrouble believing it even now....\"\nSnake, too, was puzzled. How could it \nbe possible for a few employees to \nkeep hundreds of kilograms of drugs \nhidden in Big Shell without the others \nfinding out? He couldn't afford to waste \nany more time thinking about it, though. \nThe hostages' lives were still in danger.\n\"The hostages are still in the Shell 2 \nCore, aren't they?\"\n\"Yeah....\" Emma tried to get up and \nstumbled. She slowly staggered to her \nfeet.\n\"Are you all right?\" Snake asked.\n\"Yeah, I'm fine.... Actually, the \nterrorists injected me with something. \nThey said it was to keep me from \nrunning away again. I think it was some \nkind of tranquilizer.... Now my feet feel \nlike they weigh about a ton each....\"\nEmma started to lose her balance \nagain. Snake caught her as she fell. \nEven in her incapacitated state, she'd \nmanaged to extract information from the \nenemy, and never lost sight of her \npurpose for being there. This girl was \nfull of surprises. In that respect, at \nleast, she resembled her stepbrother.\n\"That's enough. You'd better go hide \nsomewhere.\"\n\"No way. I'm coming with you.\"\n\"But...\"\n\"Please, take me with you.\" Emma's \neyes leveled with Snake's.\n\"You've saved my life twice now. I want \nto return the favor if I can. I know \nI'll come in handy somehow.... Please!\"\nSnake gave in at last. To get to the \nShell 1 Core, he'd have to cross the \nShell 1-2 connecting bridge from \nStrut D. The two set out for Strut D.#C0\n" },
    { 0x1d9489c3, "Snake made his way to the ship's \nbridge. Outside the window, beyond the \nstorm, he could see the figure of Meryl. \nHe opened the watertight door and \nstepped into the navigation room. \nMeryl turned around.\n\"You're late.\"\n\"Were you waiting for me?\"\n\"There's no one to interrupt us here. I \nneed to tell you something.\"\n\"You mean like the reason why you're \nhelping the man who killed your \nfather?\"\n\"Is that what my uncle told you?\"\nSnake nodded. Meryl shook her head.\n\"He was lying.\"\nMeryl explained.\nMatt Campbell's part in the top-secret \nMetal Gear development project in \nEldera had been to ensure the \ncountry's stability. Part of his job was \nfeeding falsified information to the \nminority separatist guerrillas, and he \nwas good at it. First, he'd pick a \nsuitable candidate. Next, he'd find his \ntarget's weak point and use it to \nblackmail him. Then he'd dangle a \ncarrot in front of the victim's eyes and \nconvince him to cooperate. Matt would \nthen send his now-willing accomplice \ninto the ranks of the separatists. Once \nthere, the spy provided the Americans \nwith useful information, selling out his \ncomrades in the process.\nBut in carrying out his mission, Matt \nmade one fatal error. In his dealings \nwith the separatists, he came to \nsympathize with their cause. He \nbecame enchanted with the idea of \nindependence. He grew to resent the \nU.S. for the way it exploited smaller, \nweaker nations for its own gain. In the \nend, Matt decided to help the guerrillas \ncreate their own nation. He wanted to \ngive them the trump card they needed \nto secure their independence. And \nwhat better gift than a bipedal, \nnuclear-capable walking tank that was \nconveniently being developed in Eldera \n(albeit secretly and quite illegally) by \nthe American military? Sergei \nGurlukovich, who was also supporting \nthe guerrillas, agreed to cooperate.\nMatt's plan was executed. The guerrillas \nsucceeded in stealing the Metal Gear \nfrom the American base. But then their \nplan was discovered. Matt was \nexecuted by the Americans. Meryl's \nfather had been punished for the crime \nof believing in his own ideals. The price \nof having a conscience was high in \nthose troubled times. In the tumult of \nthe civil war that followed, the Metal \nGear disappeared without a trace. \nHowever, six months ago, its existence \nwas reconfirmed, and the Marines had \nbeen sent to recover their long-lost \ndirty secret.\nAfter hearing the true story from \nGurlukovich, Meryl agreed to follow his \ninstructions and infiltrate the Marines. \nHer intent was to transfer the Metal \nGear over to the guerrillas through \nGurlukovich, who was still providing \nsupport to the rebels. In this way, she \ncould carry on her father's memory and \nthe ideals for which he died.\n\"Is that what Gurlukovich told you?\" \nasked Snake.\nMeryl nodded. Snake shook his head.\n\"Didn't it ever occur to you that he \nmight be lying?\"\nMeryl looked at Snake with sorrow in \nher eyes. \"...You're not going to let me \nget away, are you?\"\n\"So what are you going to do?\"\nMeryl drew her gun and pointed it at \nSnake.\n\"You're not going to shoot me,\" said \nSnake.\nShe released the safety with her thumb.\n\"Watch me.\"#C0\n" },
    { 0x3bfe641a, "Snake schloss die wasserdichte T\303\274r hinter\nsich und drehte den Griff, um sie zu\nverschlie\303\237en. Damit gewinne ich Zeit,\ndachte er. In der Zwischenzeit musste er\netwas nachpr\303\274fen. Er rief Campbell \303\274ber den\nCodec und erz\303\244hlte, was geschehen war.\n\"Meryl und Gurlukovich...\" Campbells\nStimme bebte.\n\"Meryl hat etwas vom 'letzten Willen ihres\nVaters' erw\303\244hnt. Was zum Kuckuck hatte\ndenn Ihr Bruder in diesem Land zu tun?\"\nCampbell erz\303\244hlte.\nW\303\244hrend des Kalten Kriegs, als das Land\nvon einem proamerikanischen Diktator\nregiert wurde, hatte das amerikanische\nMilit\303\244r in Eldera an einem streng\ngeheimen Projekt gearbeitet. Matt Campbell\ngeh\303\266rte zum Projektteam. Als der Kalte\nKrieg zu Ende ging, wurde auch das Projekt\nabgebrochen und die Amerikaner bereiteten\nden R\303\274ckzug aus dem Land vor. Matt\nentfernte sich aber unerlaubt von der\nTruppe. Kurze Zeit sp\303\244ter gab es einen\nbewaffneten Aufstand der\nMinderheitsgruppen des Landes gegen das\nproamerikanische Regime, das seinen\nR\303\274ckhalt verloren hatte. Eldera wurde von\neinem B\303\274rgerkrieg heimgesucht. Matts\nLeiche wurde sp\303\244ter gefunden. Hinter\ndem Aufstand standen die Spetsnaz und ihr\nKommandeur hie\303\237 Sergei Gurlukovich.\n\"Ger\303\274chten zufolge soll Matt mit\nGurlukovich zusammengearbeitet haben.\nAber das glaube ich nicht eine Minute.\"\nCampbells Worte lie\303\237en seine Annahme\ndurchblicken, dass Gurlukovich Matt get\303\266tet\nh\303\244tte.\n\"Sind Sie deshalb die ganze Zeit so von\nGurlukovich besessen gewesen?\" fragte\nSnake.\n\"Ja\", gab Campbell zu. Er hasste\nGurlukovich. Hass erzeugte Gegenhass.\n\"Waren Sie auch in das Projekt\neinbezogen?\"\n\"Nein. Es gibt eine Regel, die das nicht\nzul\303\244sst. Wir d\303\274rfen an keiner Operation\nteilnehmen, die mit Familienangeh\303\266rigen zu\ntun hat, weil das Urteilsverm\303\266gen dabei\neingeschr\303\244nkt w\303\244re.\"\n\"Ist das alles?\"\n\"Ich glaube, Matt wollte damals etwas\nAbstand gewinnen\", sagte Campbell mit\neinem bedauernden Ton. Aber er gab keine\nErkl\303\244rung f\303\274r die Gr\303\274nde der\nAktionen seines Bruders. \"Der alte Metal\nGear, den Sie gesehen haben, ist das\nEndergebnis des Projekts... Mehr wei\303\237\nich auch nicht. Die ganze Angelegenheit ist\nstreng vertraulich. Selbst heute sind noch\nalle Aufzeichnungen versiegelt.\"\nIch glaube ihm, dachte Snake. Schlie\303\237lich\nwar es ganz nat\303\274rlich, dass Menschen\nschreckliche Dinge vergessen wollten, die in\nder Vergangenheit geschehen waren. Und\nRegierungen bildeten da keine Ausnahme.\nAber manche hielten an solchen\nErinnerungen fest. Manche k\303\266nnen ihren\nHass nie vergessen. Gurlukovich und\nCampbell geh\303\266rten dazu.\n\"Snake, wir d\303\274rfen Gurlukovich und\nseine Leute nicht mit diesem Metal Gear\nentkommen lassen\", sagte Campbell. \"Er\nk\303\266nnte noch immer mit nuklearen\nSprengk\303\266pfen bewaffnet werden.\nWahrscheinlich planen sie, die\nF\303\244higkeiten des Metal Gear irgendwo\ndrau\303\237en auf See zu testen. Sie\nm\303\274ssen den Kurs des Schiffs \303\244ndern.\nGehen Sie auf die Br\303\274cke.\"#C0\n" },
    { 0xdfd966d5, "Snake si chiuse alle spalle il portello a\ntenuta stagna e gir\303\262 la maniglia di blocco.\nCos\303\254 avrebbe guadagnato tempo, pens\303\262. Nel\nfrattempo, aveva qualcosa da controllare.\nChiam\303\262 Campbell sul Codec e gli descrisse\nl'accaduto.\n\"Meryl e Gurlukovich...\" disse Campbell con\nvoce tremante.\n\"Meryl ha detto qualcosa del 'desiderio del\npadre in punto di morte'. A proposito, cosa\nci faceva suo fratello in questo Paese?\"\nCampbell gli diede la spiegazione.\nDurante la Guerra Fredda, mentre il Paese\nera sotto il controllo di un dittatore\nfiloamericano, le forze militari americane\navevano eseguito un progetto top-secret a\nEldera. Matt Campbell aveva partecipato a\nquel progetto. Finita la Guerra Fredda,\nanche il progetto fu terminato e gli americani\nsi erano preparati a lasciare il Paese, ma\nMatt si rese irreperibile. Di l\303\254 a poco, il\nregime filoamericano, avendo perso il\nsostegno, dovette affrontare insurrezioni\narmate tra le minoranze del Paese. Eldera\npiomb\303\262 cos\303\254 nella guerra civile. Pi\303\271 tardi\ntrovarono il cadavere di Matt. L'insurrezione\nera stata fomentata da Spetsnaz, al cui\ncomando era Sergei Gurlukovich.\n\"Alcuni dicono che Matt stesse\ncollaborando con Gurlukovich. Ma non ci ho\ncreduto neanche per un istante.\" Dal modo\nin cui l'aveva detto, Campbell riteneva che\nfu proprio Gurlukovich a uccidere Matt.\n\"\303\210 per questo che \303\250 stato ossessionato da\nGurlukovich per tutto questo tempo?\"\nchiese Snake.\n\"...Gi\303\240,\" ammise Campbell. Odiava\nGurlukovich. L'odio generava altro odio.\n\"Anche lei era coinvolto nel progetto?\"\n\"No. Il regolamento lo vieta. Abbiamo il\ndivieto di prendere parte a operazioni in cui\nsiano coinvolti membri della famiglia. Dicono\nche comprometterebbe il giudizio.\"\n\"Non c'\303\250 altro?\"\n\"Credo che Matt volesse mantenere le\ndistanze da me, in quel periodo.\" La voce di\nCampbell trad\303\254 del rimorso. Ma non prov\303\262 a\nspiegare per quale motivo il fratello avesse\nagito in quel modo. \"Il vecchio Metal Gear\nche hai visto \303\250 il risultato finale di quel\nprogetto... \303\210 tutto ci\303\262 che so. La faccenda \303\250\nmolto confidenziale. Le notizie sono tuttora\ntenute segrete.\"\nE ci credo, pens\303\262 Snake tra s\303\251 e s\303\251. Dopo\ntutto, era normale che la gente tentasse di\ndimenticare le cose terribili del passato, e i\nGoverni non facevano eccezione. Ma c'era\nchi rimaneva aggrappato a queste memorie:\nalcuni non dimenticarono mai l'odio, e tra\nquesti vi erano Gurlukovich e Campbell.\n\"Snake, dobbiamo impedire a Gurlukovich e\nai suoi uomini di fuggire con quel Metal\nGear,\" disse Campbell. \"Potrebbe essere\nancora armato con testate nucleari attive.\nAvranno intenzione di testare le capacit\303\240 del\nMetal Gear in alto mare. Devi cambiare la\nrotta della nave. Dirigiti verso il ponte.\"\nSnake non aveva obiezioni.#C0\n" },
    { 0xba51779e, "Somehow, the two managed to escape \nfrom Strut E onto the DE connecting \nbridge. Snake turned back towards \nStrut E, keeping an eye on their rear. \nThe terrorists would be coming after \nthem any moment now. He slapped a \nnew cartridge into the chamber of his \ngun and shouted at Emma, urging her \nto move quickly towards Strut D. She \nresponded with a nonchalant look.\n\"There's no need to be in such a hurry, \nyou know,\" she said.\nThey waited a while, but the door to \nStrut E remained shut. From the other \nside, they could hear a tumult as the \nsoldiers bashed in vain against the tightly\nsealed door. Snake turned to Emma.\n\"What did you do?\"\n\"I programmed the node to seal off all \nthe doors to Strut E once we were out \nsafely. Those guys aren't going \nanywhere.\" Emma looked extremely \nproud of herself. It seemed she'd \npicked up a thing or two from her \nexperience with Snake in Strut C.\nThey entered Strut D. Snake looked \naround for signs of the enemy, but \nthere was no one in sight. The \nterrorists must have sent the bulk of \ntheir forces to Strut E. If that were \ntrue, then the rest of the struts would \nbe nearly deserted. Rescuing the \nhostages would be a breeze.\nSnake and Emma passed through Strut \nD and emerged onto the Shell 1-2 \nconnecting bridge. Here, too, the \nenemy was nowhere to be seen. Emma \nsmiled her characteristic smile.\n\"See, no problems at all. Didn't I say \nI'd come in handy?\"\n\"I have to admit, you were pretty good \nback there,\" Snake said with a nod. He \nrecalled the way Emma had connected \nto the node using her PDA. \"By the \nway, weren't you downloading \nsomething back there?\"\n\"Yeah. Was it that obvious?\" Emma \ntook out her PDA. \"When I was \nactivating the conveyor belt, I found a \nsuspicious-looking hidden file next to \nthe conveyor belt control file. I decided \nto download it, and this is what I \nfound....\"\nThe screen of her PDA displayed the \nfile. It appeared to be some kind of note\nfrom one of the two employees who'd \nbeen killed in the accidents -- the same \nones who were helping the syndicate \nsmuggle drugs into Big Shell. In it, he \nexplained that he was only cooperating \nwith the Russians because his wife and \nchild were being held hostage. But last \nyear, he found out that they'd already \nbeen killed while trying to escape. He \nfell into a deep despair, and at one \npoint he even thought of killing himself. \nBut then he had a vision. It was still too \nearly for him to die. No, he had to get \nhis revenge on the syndicate first. \nIndeed, he'd make them pay in blood \nfor what they did to his wife and child.\nOver the course of the next year, he \ndevised an ingenious plan. He'd make it \nlook like he was killed by a rival \nsyndicate that was trying to steal the \ndrugs. This would start a war between \nthe two syndicates that would end up \ndestroying both. First, he faked his own \ndisappearance. Shortly afterward, \nhe used a crane to crush the man the \nsyndicate had sent to watch him. \nWhen the Mafia sent a man to \ninvestigate both incidents, he threw him \ninto the ocean. \nJust as he'd expected, a war erupted \nbetween the two rival syndicates. With \neverything in place, he could finally rest \nin peace. He said he was going to join \nhis wife and child. \nFinally, he implored the person reading \nthe memo to say a prayer for his \ndeparted wife and child. The file \nended there. It was dated three days \nago.\nSnake looked up from the PDA. \n\"...I guess this means the 'ghost of Big \nShell' is dead?\"\nEmma was unable to hide her shock. \n\"I... I guess so.... Actually, I \ndownloaded one more file....\"\nSnake's ears picked up a faint sound of \nsomething flying through the air. \nInstantly, he grabbed Emma and held \nher tight. There was an explosion. The \nforce of the blast knocked Snake and \nEmma off their feet.\n\"What was that?\" Emma groaned in \nSnake's arms.\nA thunderous gale of wind beat against \ntheir cheeks. Hovering over the bridge, \nbeyond the wavering heat from the \nblast, was the Harrier.\n\"Oh yeah, I guess there WAS \nsomething that could get out of Strut \nE....\" Emma muttered, a look of blank \namazement on her face. The Harrier \nstrafed the bridge with waves of \nmachine-gun fire. Snake and Emma \nfled back to the entrance to Strut D.\n\"Now what do we do?\"\n\"I guess we'll just have to kill it.\"\n\"How?\"\n\"I'll think of something.\"\n\"Are you serious?!\" Emma began to \nprotest, but before she could finish, \nSnake thrust her through the door of \nStrut D.\n\"What are you doing?\" she demanded.\n\"Stay there! No matter what happens, \ndon't come out here!\"\n\"But!\"\n\"Just listen to me for once, okay?\" \nSnake smiled at her. \"I already \npromised someone I'd bring you back \nwith me.\"\n\"Wha-\"\nThe door slammed shut.\nSnake heard the sound of a helicopter \napproaching. It was Otacon in his \nKasatka.\n\"Snake, use this!\"\nOtacon tossed down a box from the \ncockpit. Stinger missiles. With these, \nit became somewhat of an even fight. \nSnake turned and prepared to face the \nHarrier in battle.#C0\n" },
    { 0xb890009e, "Spetsnaz ? Ce sont les op\303\251rateurs\nsp\303\251ciaux du GRU sovi\303\251tique. Elle est\nrusse... ?\n" },
    { 0x0eb5ddb1, "Spetsnaz? Das ist die Spezialeinheit\ndes sowjetischen GRU.\nSie ist also Russin?\n" },
    { 0x897f3583, "Spetsnaz? Ma sono le squadre speciali\ndei GRU sovietici. Allora lei dev'essere\nrussa...\n" },
    { 0x1028bbe9, "Strut A Roof" },
    { 0x2523cbe9, "Strut E Heliport" },
    { 0x02459dff, "Strut F Warehouse" },
    { 0xc46c6555, "Strut F Warehouse " },
    { 0xf10b8605, "Strut L Perimeter - KL Connecting Bridge" },
    { 0xdd8a1594, "Su quello \"scout knife\" c'\303\250 scritto\n\"Spetsnaz\" ovunque, ma...\n" },
    { 0x30c90904, "su questo caso.\n\n- Revolver Ocelot\n\343\200\200Un ex-Spetsnaz, noto anche come\n\"Shalashaska\". Dopo il crollo dell'Unione\nSovietica, gli furono offerti dei ricchi contratti\ncome mercenario nelle regioni devastate dai\nconflitti in tutto il mondo. Le sue attivit\303\240 lo\nportarono a essere reclutato dal governo degli\nStati Uniti e a entrare in FOXHOUND. Come\n" },
    { 0x1e764316, "su questo caso.\n\343\200\200\n- Revolver Ocelot\n\343\200\200Un ex-Spetsnaz, noto anche come\n\"Shalashaska\". Dopo il crollo dell'Unione\nSovietica, gli furono offerti dei ricchi contratti\ncome mercenario nelle regioni devastate dai\nconflitti in tutto il mondo. Le sue attivit\303\240 lo\nportarono a essere reclutato dal governo degli\nStati Uniti e a entrare in FOXHOUND. Come\n" },
    { 0xe1dea0be, "Take out all enemies and get to the goal!\nBeware of the noise-making floor and\nwet floor. The mission will be terminated\nif you fall in a hole.\n" },
    { 0x84c5eb9d, "that gives preference to delaying a solution\nrather than producing one, and a hidden\nmilitary agenda to preserve what it could of\nthe old nuclear stockpile.\n\343\200\200\n\343\200\200 Richard took out several photographs from\nthe folder and handed them to me. They all\nappeared to be satellite captures of the\nnuclear weapons disposal plant on Shadow\nMoses Island, perhaps acquired from the NRO\n" },
    { 0xb5a7fabe, "The passageway ahead leads to|a hangar. He'll be waiting there." },
    { 0xe26cca42, "The Singularity" },
    { 0xd4e44a45, "Ton SOCOM poss\303\250de un silencieux.\n" },
    { 0x53c52f02, "Tras matar a los rehenes con su bomba,\nFatman me ha elegido como su pr\303\263ximo \nobjetivo. Su cuerpo es enorme... no tengo\nm\303\241s remedio que acabar con \303\251l.\n" },
    { 0x58c70e64, "triggers cell death.\343\200\200The TNF-alpha travels\nthrough the blood stream to the heart, where\nit binds to the receptors of cardiac cells.\"\n\343\200\200 \"And that causes a heart attack?\"\n\343\200\200 \"The affected cells undergo rapid apoptosis.\nAnd the owner of that heart - dies.\"\n\343\200\200 \"Apoptosis -- I remember that. Programmed\ncell death for damaged cells.\" Snake\nmurmured. The tense silence descended once\nagain.\n" },
    { 0xef8ff184, "Tu SOCOM tiene un silenciador.\n" },
    { 0x9e76b410, "Un couteau de scout plein de surprises. |Tu es une Spetsnaz ?" },
    { 0x206908ae, "Vorsicht, er ist in|diesem Bereich." },
    { 0x01ebe041, "Waffenmodus SOCOM-Ebene 01\n" },
    { 0xf24ead03, "Weapon Mode SOCOM Level 01\n" },
    { 0x00f857ac, "Wenn Du einen Schalld\303\244mpfer auf die\nSOCOM aufsetzt, h\303\266rt man die Sch\303\274sse\nkaum. Du solltest Dir einen suchen.\n" },
    { 0x4978c6fb, "You're Jennifer." },
    { 0x05c411f9, "You're... a man...?" },
    { 0x535241a5, "\302\277Spetsnaz? Se trata de las operaciones\nespeciales de la GRU sovi\303\251tica.\n\302\277As\303\255 que debe ser rusa...?\n" },
    { 0xf1f203c4, "\303\240 cause du blocage du syst\303\250me|de rotation du barillet. " },
    { 0x7972e8de, "\343\200\200\nRevolver Ocelot\nUn antiguo Spetsnaz, tambi\303\251n conocido como\n\"Shalashaska\". Tras el derrumbamiento de la\nUni\303\263n Sovi\303\251tica, trabaj\303\263 como mercenario a\nsueldo en diferentes conflictos regionales del\nmundo. Tras probar sus habilidades, fue\nreclutado por el gobierno de los Estados Unidos,\ny finalmente pas\303\263 a formar parte de FOXHOUND.\nTal y como su nombre en clave indica, es un\n" },
    { 0xfb5e531d, "\343\200\200 \"I was picked up in Northern Rhodesia, in\nthe '80s. I was an orphan.\"\n\343\200\200 \"Rhodesia? During all the guerrilla\nwarfare?\"\n\343\200\200 \"Zimbabwe used to be a British colony, you\nknow. There was a sizable Indian population\nthere then. Maybe that's where I get the color\nof my skin, but I'm not even sure of that.\"\n\343\200\200 \"Naomi, why dwell on the past? If you can\nunderstand who you are now, isn't that all that\n" },
    { 0x6d8e4218, "\343\200\200 \"You couldn't understand that either.\nSomeone who got the chance to kill his own\nfather wouldn't! You managed to deprive me\neven of that revenge. But I will accomplish\nwhat our father dreamt of and never achieved.\nThat's how I'll kill him -- by surpassing him.\"\nLiquid ended his proclamation by leaping into\nMetal Gear's cockpit. Snake fired a burst from\nhis SOCOM, but the bullets ricocheted off Metal\nGear's armor.\n" },
    { 0x9e0be742, "\343\202\252\345\244\225\343\202\263\343\203\263#T{}\343\200\201\343\201\212\345\211\215\343\201\256\350\250\200\343\201\206\351\200\232\343\202\212\343\203\232\343\203\263\343\202\277\343\202\274\343\203\237\343\203\263\n\343\202\222\351\243\262\343\202\223\343\201\240\343\202\211\350\210\271\346\217\272\343\202\214\343\201\214\346\255\242\343\201\276#T{}\343\201\243\343\201\237\343\201\236#T{}\343\200\202\n" },
    { 0xfbc0e66c, "\343\203\232\343\203\263\343\202\277\343\202\274\343\203\237\343\203\263|\n\343\203\231\343\203\263\343\202\276\343\202\270\343\202\242\343\202\274\343\203\224\343\203\263\347\263\273\346\212\227\344\270\215\345\256\211\350\226\254#T{}\343\200\202|\n\346\212\227\344\270\215\345\256\211\344\275\234\347\224\250\344\273\245\345\244\226\343\201\253\347\255\213\345\274\233\347\267\251\344\275\234\347\224\250#T{}\343\200\201|\n\350\207\252\345\276\213\347\245\236\347\265\214\350\252\277\347\257\200\344\275\234\347\224\250\347\255\211\343\202\202\343\201\202\343\202\213#T{}\343\200\202|\n##B{}{[CIRCLE]#T{}\343\200\201\346\261\272\345\256\232#T{}}#\343\203\234\345\244\225\343\203\263\343\201\247\344\275\277\347\224\250#T{}\343\200\202" },
    { 0xc3fdef71, "\343\203\232\343\203\263\343\202\277\343\202\274\343\203\237\343\203\263|\n\343\203\231\343\203\263\343\202\276\343\202\270\343\202\242\343\202\274\343\203\224\343\203\263\347\263\273\346\212\227\344\270\215\345\256\211\350\226\254#T{}\343\200\202|\n\347\213\231\346\222\203\351\212\203\343\202\222\346\247\213\343\201\210\343\201\237\346\231\202\343\201\256\346\211\213\343\201\266\343\202\214\343\202\222|\n\344\270\200\345\256\232\346\231\202\351\226\223\346\255\242\343\202\201\343\202\213#T{}\343\200\202\343\202\246#T{}\343\202\243\343\203\263\343\203\211\343\202\246\343\201\247|\n\351\201\270\343\202\223\343\201\247##B{}{[CIRCLE]#T{}\343\200\201\346\261\272\345\256\232#T{}}#\343\203\234\345\244\225\343\203\263\343\201\247\346\234\215\347\224\250#T{}\343\200\202" },
    { 0x0361d625, "\343\203\232\343\203\263\343\202\277\343\202\274\343\203\237\343\203\263\343\201\257\343\201\206\343\201\244\347\227\205\343\202\204\350\207\252\345\276\213\347\245\236\347\265\214\345\244\261\350\252\277\347\227\207\n\343\201\256\346\262\273\347\231\202\343\201\253\344\275\277\343\202\217\343\202\214\343\202\213\343\203\231\343\203\263\343\202\276\343\202\270\343\202\242\343\202\274\343\203\224\343\203\263\347\263\273\343\201\256\n\346\212\227\344\270\215\345\256\211\350\226\254\343\201\240#T{}\343\200\202\n" },
    { 0xafa5079d, "\343\203\232\343\203\263\343\202\277\343\202\274\343\203\237\343\203\263\343\201\257\351\254\261\347\227\205\343\201\250\343\201\213\350\207\252\345\276\213\347\245\236\347\265\214\345\244\261\350\252\277\347\227\207#T{}\343\200\201\n\344\270\215\345\256\211\347\245\236\347\265\214\347\227\207\343\201\252\343\202\223\343\201\213\343\201\256\346\262\273\347\231\202\343\201\253\344\275\277\343\202\217\343\202\214\343\202\213\n\343\203\236\343\202\244\343\203\212\343\203\274\343\203\273\343\203\210\343\203\251\343\203\263\343\202\255\343\203\251\343\202\244\343\202\266\343\203\274\343\201\240#T{}\343\200\202\n" },
    { 0x04404040, "\343\203\232\343\203\263\343\202\277\343\202\274\343\203\237\343\203\263\343\202\222\344\275\277\343\201\210\343\201\260\343\201\227\343\201\260\343\202\211\343\201\217\343\201\256\351\226\223\n\346\211\213\343\201\266\343\202\214\343\202\222\346\255\242\343\202\201\343\202\213\343\201\223\343\201\250\343\201\214\343\201\247\343\201\215\343\202\213#T{}\343\200\202\n\347\262\276\345\257\206\343\201\252\347\213\231\346\222\203\343\201\214\343\201\227\343\202\204\343\201\231\343\201\217\343\201\252\343\202\213\343\201\257\343\201\232\343\201\240#T{}\343\200\202\n" },
    { 0x63357926, "\343\203\232\343\203\263\343\202\277\343\202\274\343\203\237\343\203\263\343\202\222\346\214\201#T{}\343\201\243\343\201\246\343\202\213\343\201\252#T{}\343\200\202\343\202\246#T{}\343\202\243\343\203\263\343\203\211\343\202\246\n\343\201\247\351\201\270\343\202\223\343\201\247##B{}{[CIRCLE]#T{}\343\200\201\346\261\272\345\256\232#T{}}#\343\203\234\345\244\225\343\203\263\343\201\247\344\275\277\343\201\210\343\201\260\347\213\231\346\222\203\351\212\203\343\202\222\n\346\211\261\343\201\206\346\231\202\343\201\256\346\211\213\343\201\266\343\202\214\343\202\222\346\255\242\343\202\201\343\202\213\343\201\223\343\201\250\343\201\214\343\201\247\343\201\215\343\202\213#T{}\343\200\202\n" },
    { 0x65eb4a7b, "\343\203\232\343\203\263\343\202\277\343\202\274\343\203\237\343\203\263\343\202\222\350\243\205\345\202\231\343\201\227\343\201\246\343\202\213\343\201\255#T{}\343\200\202\n\343\202\242\343\202\244\343\203\206\343\203\240\343\202\246#T{}\343\202\243\343\203\263\343\203\211\343\202\246\343\201\247\351\201\270\343\202\223\343\201\247\n##B{}{[CIRCLE]#T{}\343\200\201\346\261\272\345\256\232#T{}}#\343\203\234\345\244\225\343\203\263\343\202\222\346\212\274\343\201\233\343\201\260\344\275\277\343\201\206\343\201\223\343\201\250\343\201\214\343\201\247\343\201\215\343\202\213#T{}\343\200\202\n" },
    { 0x0aa66f91, "\345\205\250\343\201\246\343\201\256\346\250\231\347\232\204\343\202\222\347\240\264\345\243\212\343\201\227#T{}\343\200\201\343\202\264\343\203\274\343\203\253\343\201\233\343\202\210#T{}\357\274\201\n\347\213\231\346\222\203\351\212\203\343\202\222\346\247\213\343\201\210\343\201\237\346\231\202\343\201\256\346\211\213\343\201\266\343\202\214\343\201\257\343\201\227#T{}\343\202\203\343\201\214\343\201\277\347\212\266\346\205\213\343\202\204\n\343\203\233\343\203\225\343\202\257\347\212\266\346\205\213\343\201\247\343\201\257\345\260\217\343\201\225\343\201\217\343\201\252\343\202\213#T{}\343\200\202\n\343\203\232\343\203\263\343\202\277\343\202\274\343\203\237\343\203\263\343\202\202\343\201\206\343\201\276\343\201\217\344\275\277\343\201\210#T{}\343\200\202\n" },
    { 0xa5229bef, "\345\210\245\343\201\253\343\203\232\343\203\263\343\202\277\343\202\274\343\203\237\343\203\263\350\207\252\344\275\223\343\201\253\350\210\271\351\205\224\343\201\204\343\202\222\346\255\242\343\202\201\343\202\213\n\345\212\271\350\203\275\343\201\214\343\201\202\343\202\213\350\250\263\343\201\230#T{}\343\202\203\343\201\252\343\201\204#T{}\343\200\202\346\206\266\343\201\210\343\201\246\343\201\212\343\201\221\343\202\210#T{}\343\200\202\n" },


                     // override fixes

    { 0x42c9bc0e, "#\356\200\201{\343\203\232\343\203\263\343\202\277\343\202\274\343\203\237\343\203\263\356\200\200\343\200\201\345\256\211\345\256\232\345\211\244\356\200\200}#\343\202\222\344\275\277\343\201\210\343\201\260\343\201\227\343\201\260\343\202\211\343\201\217\343\201\256\351\226\223\n\346\211\213\343\201\266\343\202\214\343\202\222\346\255\242\343\202\201\343\202\213\343\201\223\343\201\250\343\201\214\343\201\247\343\201\215\343\202\213\356\200\200\343\200\202\n\347\262\276\345\257\206\343\201\252\347\213\231\346\222\203\343\201\214\343\201\227\343\202\204\343\201\231\343\201\217\343\201\252\343\202\213\343\201\257\343\201\232\343\201\240\356\200\200\343\200\202" },
    { 0xfea1ba42, "\343\203\232\343\203\263\343\202\277\343\202\274\343\203\237\343\203\263|\n\343\203\231\343\203\263\343\202\276\343\202\270\343\202\242\343\202\274\343\203\224\343\203\263\347\263\273\346\212\227\344\270\215\345\256\211\350\226\254\356\200\200\343\200\202|\n\346\212\227\344\270\215\345\256\211\344\275\234\347\224\250\344\273\245\345\244\226\343\201\253\347\255\213\345\274\233\347\267\251\344\275\234\347\224\250\356\200\200\343\200\201|\n\350\207\252\345\276\213\347\245\236\347\265\214\350\252\277\347\257\200\344\275\234\347\224\250\347\255\211\343\202\202\343\201\202\343\202\213\356\200\200\343\200\202|\n#\356\200\201{\356\202\212\356\200\200\343\200\201\346\261\272\345\256\232\356\200\200}#\343\201\247\344\275\277\347\224\250\356\200\200\343\200\202" },
    { 0x4d9c3b1d, "\343\203\232\343\203\263\343\202\277\343\202\274\343\203\237\343\203\263|\n\343\203\231\343\203\263\343\202\276\343\202\270\343\202\242\343\202\274\343\203\224\343\203\263\347\263\273\346\212\227\344\270\215\345\256\211\350\226\254\356\200\200\343\200\202|\n\347\213\231\346\222\203\351\212\203\343\202\222\346\247\213\343\201\210\343\201\237\346\231\202\343\201\256\346\211\213\343\201\266\343\202\214\343\202\222|\n\344\270\200\345\256\232\346\231\202\351\226\223\346\255\242\343\202\201\343\202\213\356\200\200\343\200\202\343\202\246\356\200\200\343\202\243\343\203\263\343\203\211\343\202\246\343\201\247|\n\351\201\270\343\202\223\343\201\247#\356\200\201{\356\202\212\356\200\200\343\200\201\346\261\272\345\256\232\356\200\200}#\343\201\247\346\234\215\347\224\250\356\200\200\343\200\202" },
    { 0x842c1881, "Cardboard Box 5\nCardboard box to transport\ngame software. The eye design\nis \"eye-catching.\"\nEquip to wear." },
};

constexpr CaptionOverride kMGS3CaptionTypoFixes[] =
{
    { 0x108508f7, "... hay uno entre nosotros..." },
    { 0x3ce2c9cf, "Cardboard Box B:\nEquip to wear.\nSays \"To the Weapons Lab:\nHangar\" on the side.\n" },
    { 0x3cc3812d, "der gro\303\237en Festung in Groznyj Grad, trifft," },
    { 0x1284b292, "Eine echte Patriotin." },
    { 0xc67314f3, "Enviando una unidad de\napoyo. Extremad la cautela." },
    { 0xd38ec246, "Estoy bien. Solo con estes dos doble\npapeles," },
    { 0xf2b1d7e7, "Hmm ... Nur ein Frosch." },
    { 0x0db65894, "H\303\266r auf, auf mich zu\nschie\303\237en!" },
    { 0xfa36c122, "Ihr Leben lag in deinen H\303\244nden ..." },
    { 0x26f99527, "Ja. Aus irgendeinem Grund f\303\274hlt sich\nso ... nostalgisch an.\n" },
    { 0x2efbd9f7, "Los viajes espaciales son s\303\263lo otra forma\nde participar en la carrera por el poder" },
    { 0x40e3dd35, "Mein Kriegsgl\303\274ck scheint sich endlich\ngewendet zu haben." },
    { 0xf8271dfa, "Not as a soldier," },
    { 0x0591814c, "OK, let's go." },
    { 0x9c8e98fc, "Shagohod bedeutet ..." },
    { 0x91dd8cd9, "\302\241Es m\303\255o!" },
    { 0x062690cd, "\302\277\302\241Qu\303\251 ha sido ese\nruido!?" },

};

// Hashes that should return the original PS2 string
// Mostly censorship/licensing stuff that got changed in HDC.
constexpr uint32_t kMGS2ForceOriginalHashes[] =
{
    0x8c16953d, //"Bei diesem Spiel wird Metal Gear Solid | für die Konsole"
    0xe34e7619, //"Busca la foto del tipo de chica preferida de.\nMasahiro Hinami \nJuego representativo: Ring of Red"
    0xe1afbf50, //"Busca la foto del tipo de chica preferida de.\nSadaaki Kaneyoshi \nJuegos en los que ha trabajado: \nlas series de GuitearFreaks, \nMetal Gear Solid 2 (unidad de guión)"
    0x372cbc40, //"Busca la foto del tipo de chica preferida de.\nShinta Nojiri \nJuegos en los que ha trabajado: \nMetal Gear (GB), \nMetal Gear Solid 2 (unidad del guión)"
    0xab85e934, //"Busca la foto del tipo de chica preferida de.\nYoshikazu Matsuhana \nJuegos en los que ha trabajado: Snatcher, \nPolicenauts, Lethal Enforcers, \nlas series de Metal Gear Solid"
    0x16c4e282, //"Busca la foto del tipo de chica preferida de.\nYutaka Negishi \nJuegos en los que ha trabajado: \nMetal Gear Solid 2\n(Director Artístico del Escenario de Fondo)"
    0x0afbedee, //"Dans ce jeu, Metal Gear Solid |pour Console"
    0xec7e49c4, //"Find the photograph of the girl of the\ndeveloper's type!\nMasahiro Hinami \nRepresentative title: Ring of Red"
    0x2604b847, //"Find the photograph of the girl of the\ndeveloper's type!\nSadaaki Kaneyoshi \nTitles worked on: GuitarFreaks series,\nMetal Gear Solid 2 (Script unit)"
    0xb87f8c62, //"Find the photograph of the girl of the\ndeveloper's type!\nShinta Nojiri \nTitles worked on: Metal Gear (GB),\nMetal Gear Solid 2 (Script unit)"
    0x6668cbba, //"Find the photograph of the girl of the\ndeveloper's type!\nYoshikazu Matsuhana \nTitles worked on: Snatcher, Policenauts,\nLethal Enforcers, Metal Gear Solid series"
    0x0d56f4a2, //"Find the photograph of the girl of the\ndeveloper's type!\nYutaka Negishi\nTitles worked on: Metal Gear Solid 2\n(Background Art Director)"
    0xee8b3c11, //"Finden Sie das Foto vom Frauentyp\ndes Entwicklers!\nMasahiro Hinami \nRepräsentativer Titel: Ring of Red"
    0xc210ac50, //"Finden Sie das Foto vom Frauentyp\ndes Entwicklers!\nSadaaki Kaneyoshi \nMitarbeit bei: Guitar-Freaks-Serie,\nMetal Gear Solid 2 (Skript-Einheit)"
    0x33691fc6, //"Finden Sie das Foto vom Frauentyp\ndes Entwicklers!\nSadaaki Kaneyoshi \nMitarbeit bei: GuitarFreaks Serie,\nMetal Gear Solid 2 (Skripteinheit)"
    0x3dbc57c6, //"Finden Sie das Foto vom Frauentyp\ndes Entwicklers!\nShinta Nojiri \nMitarbeit bei: Metal Gear (GB),\nMetal Gear Solid 2 (Skript-Einheit)"
    0xc33e21d0, //"Finden Sie das Foto vom Frauentyp\ndes Entwicklers!\nShinta Nojiri \nMitarbeit bei: Metal Gear (GB),\nMetal Gear Solid 2 (Skripteinheit)"
    0x24bb5310, //"Finden Sie das Foto vom Frauentyp\ndes Entwicklers!\nYoshikazu Matsuhana \nMitarbeit bei: Snatcher, Policenauts,\nLethal Enforcers, Metal Gear Solid Serie"
    0x87ca5004, //"Finden Sie das Foto vom Frauentyp\ndes Entwicklers!\nYutaka Negishi \nMitarbeit bei: Metal Gear Solid 2\n(Background Art Director)"
    0x39a96c1e, //"Finden Sie das Foto vom Frauentyp\ndes Entwicklers!\nYutaka Negishi \nMitarbeit bei: Metal Gear Solid 2\n(künstl. Leiter Hintergrund)"
    0xd71196b4, //"In questo gioco, chiameremo |Metal Gear Solid per console"
    0x6f56447e, //"In this game, Metal Gear Solid|for the console..."
    0xc82ba1be, //"nos estaremos refiriendo |a Metal Gear Solid para Consola."
    0x51afc248, //"Trouvez la photographie de la fille \ndu type du développeur !\nMasahiro Hinami \nTitre représentatif : Ring of Red"
    0x0636cf9c, //"Trouvez la photographie de la fille \ndu type du développeur !\nSadaaki Kaneyoshi \nA travaillé sur les titres suivants : \nMetal Gear Solid 2 (unité Scénario)"
    0xe36faa00, //"Trouvez la photographie de la fille \ndu type du développeur !\nShinta Nojiri \nA travaillé sur les titres suivants : \nMetal Gear Solid 2 (unité Scénario)"
    0x40ccce5a, //"Trouvez la photographie de la fille \ndu type du développeur !\nYoshikazu Matsuhana \nA travaillé sur les titres suivants : \nPolicenauts, Metal Gear Solid series"
    0x4f9aa354, //"Trouvez la photographie de la fille \ndu type du développeur !\nYutaka Negishi \nA travaillé sur les titres suivants : \nMetal Gear Solid 2 (directeur artistique)"
    0x3a2508c7, //"Trova la foto della ragazza che va\na genio allo sviluppatore!\nMasahiro Hinami \nTitolo rappresentativo: Ring of Red"
    0x7e72a17e, //"Trova la foto della ragazza che va\na genio allo sviluppatore!\nSadaaki Kaneyoshi \nTitoli a cui ha lavorato: serie GuitarFreaks,\nMetal Gear Solid 2 (unità Script)"
    0x0b48bc55, //"Trova la foto della ragazza che va\na genio allo sviluppatore!\nShinta Nojiri \nTitoli a cui ha lavorato: Metal Gear (GB),\nMetal Gear Solid 2 (unità Script)"
    0xe709a65d, //"Trova la foto della ragazza che va\na genio allo sviluppatore!\nYoshikazu Matsuhana \nTitoli a cui ha lavorato: Policenauts, \nSnatcher, serie Metal Gear Solid"
    0x14ec1798, //"Trova la foto della ragazza che va\na genio allo sviluppatore!\nYutaka Negishi \nTitoli a cui ha lavorato: Metal Gear Solid 2\n(direttore artistico sfondi)"
    0xf0a11858, //"制作者の好みを暴け！\n根岸　豊\n「メ夕ルギアソリッド2」(デザインリーダー)"
    0x57544e4f, //"制作者の好みを暴け！兼吉完聡\n主な作品は「ギ夕ーフリークス」シリーズ。\n「メ夕ルギアソリッド2」(スクリプト班)"
    0x7ca2016f, //"制作者の好みを暴け！日並雅宏\n代表作は「リングオブレッド」。"
    0xc76fa726, //"制作者の好みを暴け！松花賢和\n主な作品は「スナッチャー」「ポリスノーツ」\n「リーサルエンフォーサーズ」\n「メ夕ルギアソリッド」シリーズ。"
    0xaf473797, //"制作者の好みを暴け！野尻真太\n主な作品は「ゴーストバベル」\n「メ夕ルギアソリッド2」(スクリプト班)"


};

#pragma region mgs2_ps2_memory_card_strings
    // disable bp's overrides for these hashes, forcing the original ps2 string for stuff like memory cards
constexpr uint32_t kMGS2ForcePs2StringHashes[] =
{
    0x3ce25ce0,     //"Aucun périphérique de stockage n'a\nété détecté| un minimum de ?ko d'espace\nlibre est requis pour sauvegarder la partie.\nvoulez-vous commencer à jouer?"
    0x15a6acb5,     //"Aucun périphérique de stockage n'a été détecté."
    0x8c16953d,     //"Bei diesem Spiel wird Metal Gear Solid|für die Konsole"
    0xd15de54d,     //"Collega subito la tua \nperiferica di memorizzazione!"
    0x8b315845,     //"Connect your storage device!"
    0x1c2fb5c0,     //"Connecte ton périphérique de stockage tout de suite !"
    0x0afbedee,     //"Dans ce jeu, Metal Gear Solid |pour Console"
    0x75207d2a,     //"Devi avere una\nperiferica di memorizzazione."
    0x4d750d46,     //"Du benötigst ein Speichergerät,\num speichern zu können."
    0xa7e77c85,     //"Du benötigst ein Speichergerät."
    0xd326eaa3,     //"Du hast kein Speichergerät!"
    0xcde61b57,     //"Du kannst nur mit einem eigenen Speichergerät speichern."
    0xeae9ef8e,     //"Ecoute, il est impossible de sauvegarder si\nle périphérique de stockage n'est pas\nformaté !"
    0xe45f246d,     //"Ehi, non è stata rilevata alcuna\nperiferica di memorizzazione."
    0xf3d03a8b,     //"El dispositivo de almacenamiento no está formateado.|\n¿Formatear y guardar?"
    0x44a0e5d7,     //"Error al detectar el dispositivo de almacenamiento.\nInténtalo de nuevo."
    0xa1c371c4,     //"Error detecting the storage device.\nPlease try again."
    0x7f0f0756,     //"Errore nel rilevamento della\nperiferica di memorizzazione.\nRiprova."
    0x57685ac3,     //"Errore nel rilevamento della|\nperiferica di memorizzazione.|\nPer salvare il gioco occorrono|\nalmeno ?KB di spazio libero.|\nVuoi iniziare il gioco così com'è?"
    0x0f59b998,     //"Espace insuffisant sur le périphérique de stockage."
    0x91177588,     //"Est- ce que tu peux connecter un\npériphérique de stockage ?"
    0xd4daf2bb,     //"Fehler beim Erkennen des Speichergeräts.\nBitte erneut versuchen."
    0x1575d0a8,     //"Fehler beim Lesen des|\nSpeichergeräts.|\nZum Speichern sind mind.|\n?KB freier Speicherplatz nötig.|\nMöchten Sie das Spiel so beginnen?"
    0xb988bcff,     //"Ha habido un error al detectar|\nel dispositivo de almacenamiento.|\nPara guardar el juego necesitas al menos|\n?KB libres. ¿Quieres comenzar a jugar|\nde todas las formas?"
    0xf429ff51,     //"He, es wurde kein Speichergerät erkannt."
    0x22a271b5,     //"Hey, there is no storage device."
    0x7174e9be,     //"Hé, aucun périphérique de stockage n'a été détecté !"
    0x5cfe14eb,     //"I can't save unless you have a\nstorage device!"
    0x1f34eca9,     //"I'm sorry, Jack, but there isn't\nenough free space on the\nstorage device."
    0x0d88e1da,     //"Ich kann erst speichern, wenn du\nein Speichergerät hast!"
    0x26d2259d,     //"Ich kann erst speichern, wenn Du das\nSpeichergerät formatiert\nhast!"
    0x2b4c0e51,     //"Il est impossible de sauvegarder si tu n'as\npas connecté de périphérique de stockage."
    0xa5efc33c,     //"Il n'y a pas assez d'espace sur le|\npériphérique de stockage.|\nUn minimum de ?Ko d'espace libre|\nest requis pour sauvegarder la partie.|\nVoulez-vous commencer à jouer?"
    0x40601fe7,     //"Il n'y a pas de périphérique de stockage !"
    0x6bc62975,     //"Il te faut un périphérique de stockage."
    0xd71196b4,     //"In questo gioco, chiameremo |Metal Gear Solid per console"
    0x6f56447e,     //"In this game, Metal Gear Solid|for the console..."
    0xf9187857,     //"Insufficient space on the storage device."
    0x9c457596,     //"Jack, aucun périphérique de stockage n'a été détecté."
    0xfa80bf39,     //"Jack, es wurde kein Speichergerät erkannt."
    0xbdf32a2c,     //"Jack, no se ha detectado ningún dispositivo de almacenamiento."
    0xafc5a512,     //"Jack, non è stata rilevata alcuna\nperiferica di memorizzazione."
    0xf64ba020,     //"Jack, there's no storage device detected"
    0x877870fc,     //"Je regrette, Jack, mais il n'y a\npas assez d'espace libre sur\nle périphérique de stockage."
    0x1e9aa71f,     //"Lo siento Jack, pero no hay suficiente\nespacio libre en el dispositivo de almacenamiento."
    0x06efaac5,     //"Look, there's no way to save\nunless you format the storage device!"
    0x0d3c297b,     //"Mi spiace, Jack, ma non c'è abbastanza\nspazio disponibile sulla periferica di\nmemorizzazione."
    0x4d15a4e4,     //"Necesitas conectar un\ndispositivo de almacenamiento."
    0xc3f165c5,     //"Nessun periferica di memorizzazione|\nrilevata.|\nPer salvare il gioco occorrono|\nalmeno ?KB di spazio libero.|\nVuoi iniziare il gioco così com'è?"
    0xd699db35,     //"Nicht genug Speicherplatz auf dem Speichergerät."
    0x385f09c4,     //"Nicht genug Speicherplatz|\nauf dem Speichergerät.\nZum Speichern sind mind.|\n?KB freier Speicherplatz nötig.|\nMöchten Sie das Spiel so beginnen?"
    0xe2d49989,     //"No hay suficiente espacio libre en el dispositivo de almacenamiento."
    0x993684d7,     //"No hay suficiente espacio libre en el dispositivo de almacenamiento.|\nPara guardar el juego necesitas al menos|\n?KB libres. ¿Quieres comenzar a jugar|\nde todas las formas?"
    0x81893209,     //"No puedes guardar a menos\nque conectes un\ndispositivo de almacenamiento."
    0x745fb136,     //"No se ha detectado ningún dispositivo de almacenamiento."
    0xab9d8915,     //"No se ha detectado ningún dispositivo de almacenamiento.|\nPara guardar el juego necesitas al menos|\n?KB libres. ¿Quieres comenzar a jugar|\nde todas las formas?"
    0x80055523,     //"No storage device|\ndetected. At least ?KB of free space|\nis needed to save your game.|\nWill you start the game as it is?"
    0x978442f0,     //"Non c'è alcuna periferica\ndi memorizzazione!"
    0xe17f3f91,     //"Non posso salvare se non hai una\nperiferica di memorizzazione!"
    0xe0d3776f,     //"Non puoi salvare senza avere una\nperiferica di memorizzazione pronta."
    0xc82ba1be,     //"nos estaremos refiriendo |a Metal Gear Solid para Consola."
    0x8fc57a7f,     //"Oh nein, Snake, es war nicht genug\nSpeicherplatz auf dem Speichergerät, um den Spielstand zu\nspeichern."
    0x53abf359,     //"Oh no, Snake! Non avevi abbastanza \nspazio disponibile sulla periferica di\nmemorizzazione per salvare il gioco."
    0xa5fc0e5a,     //"Oh no. Snake, there wasn't enough\nspace left on the storage device."
    0xdbe96c80,     //"Oh, no. Snake, no hay espacio\nsuficiente en el dispositivo de\nalmacenamiento para guardar la partida."
    0xb6ae26c7,     //"Oh, no. Snake, no hay espacio suficiente\nen el dispositivo de almacenamiento\npara guardar la partida."
    0x4b9dc4ac,     //"Oh, non ! Snake, il n'y avait plus assez\nd'espace libre sur le périphérique de stockage\npour sauvegarder la partie."
    0x36192b62,     //"Para guardar necesitas\nun dispositivo de almacenamiento."
    0x8d189145,     //"Periferica di memorizzazione non formattata.|\nVuoi formattare e salvare?"
    0xe7afcdbb,     //"Periferica di memorizzazione non rilevata."
    0x02f6745e,     //"Périphérique de stockage non formaté.|\nFormater et sauvegarder?"
    0xaf39abb5,     //"Saving can't be done unless you have\na storage device."
    0x8587ae98,     //"Schließe jetzt dein Speichergerät an!"
    0xc3bc2be6,     //"Senti, non c'è modo di salvare se non\nformatti la periferica di memorizzazione!"
    0xe2470575,     //"Si tu n'as pas connecté\nde périphérique de stockage,\nje ne peux pas sauvegarder, moi !"
    0x76b089ef,     //"Snake, es war nicht genug\nSpeicherplatz auf dem Speichergerät!"
    0x48b538ec,     //"Snake, non c'era abbastanza\nspazio disponibile sulla\nperiferica di memorizzazione!"
    0x3cafef16,     //"Snake, there wasn't enough\nspace left on the storage device!"
    0xf0c9d64b,     //"Sorry, Jack, aber Du hast\nnicht genug freien Speicherplatz\nauf dem Speichergerät."
    0x02420058,     //"Spazio libero insufficiente sulla\nperiferica di memorizzazione."
    0x694724d8,     //"Spazio libero insufficiente sulla|\nperiferica di memorizzazione per|\ncreare un nuovo file di salvataggio.|\nPer salvare il gioco occorrono|\nalmeno ?KB di spazio libero.|\nVuoi iniziare il gioco così com'è?"
    0xa2ad510b,     //"Speichergerät nicht formatiert.|\nFormatieren und speichern?"
    0x46261252,     //"Speichergerät wurde nicht erkannt."
    0x7bcd1c43,     //"Speichergerät wurde nicht erkannt."
    0x29c23af2,     //"Speichergerät wurde nicht|\nerkannt. Zum Speichern sind mind.|\n?KB freier Speicherplatz nötig.|\nMöchten Sie das Spiel so beginnen?"
    0x6916e3b4,     //"Storage device not detected."
    0x58a923b9,     //"Storage device not formatted.|\nFormat and save?"
    0x8308622f,     //"There is insufficient space on the|\nstorage device to create a new|\nsave file. At least ?KB of free space|\nis needed to create a new save file.|\nWill you start the game as it is?"
    0x3bbd25c8,     //"There is insufficient space|\non the storage device.\nAt least ?KB of free space|\nis needed to save your game.|\nWill you start the game as it is?"
    0xeece71b1,     //"There was an error when detecting|\nthe storage device.|\nAt least ?KB of free space|\nis needed to save your game.|\nWill you start the game as it is?"
    0xf3e654d0,     //"There's no storage device!"
    0x8816b3ea,     //"Ti serve una periferica di memorizzazione\nsu cui salvare i dati."
    0xab2184a3,     //"Tu ne peux pas enregistrer sans\npériphérique de stockage."
    0x5ed5dd14,     //"Une erreur s'est produite pendant|\nla détection du périphérique de stockage.|\nUn minimum de ?Ko d'espace libre|\nest requis pour sauvegarder la partie.|\nVoulez-vous commencer à jouer?"
    0x3332ef15,     //"You need a storage device\nto save."
    0xd5d607a1,     //"You need to have a storage device."
    0x08c919f8,     //"¡Conecta el dispositivo de almacenamiento!"
    0xf97a4596,     //"¡Eh!, no se ha detectado ningún\ndispositivo de almacenemiento."
    0x141bf8f9,     //"¡Mira, no hay forma de guardar a menos\nque fomatees el dispositivo de almacenamiento!"
    0x47920e81,     //"¡No hay ningún dispositivo de almacenamiento!"
    0x5be0b79e,     //"¡No puedo guardar a menos que\nconectes un dispositivo de almacenamiento!"
    0x2232aa29,     //"ストレージが見つかりません。"
    0x423fa817,     //"ストレージの空き容量が足りません。"
};

#pragma endregion mgs2_ps2_memory_card_strings

#pragma region mgs3_ps2_memory_card_strings
constexpr uint32_t kMGS3ForcePs2StringHashes[] =
{
    0xa03ad401,		//"Alors , connecte un périphérique de stockage"
    0x9a170dc9,		//"An error occurred while saving camouflage\ndata to storage device.\nTry saving again?"
    0x7ce85b7f,		//"At least %dKB of free space is needed\non storage device to save camouflage\ndata. Continue?"
    0x059276f1,		//"Aucun périphérique de stockage\nn'a été détecté."
    0xea58eaa6,		//"Aucun périphérique de stockage n'a été détecté."
    0xf6d886ef,		//"Aucune donnée de chargement n'a été\ndétectée. Insérez le périphérique de stockage\nutilisée pour charger les données."
    0x527058d2,		//"Aucune périphérique de stockage\nn'a été détecté.\nContinuer la partie ?"
    0x4c93398b,		//"Aucune périphérique de stockage\nn'a été détecté.\nContinuer sans sauvegarder?"
    0x8db31603,		//"Aucune périphérique de stockage détecté. Un\nminimum de %dKB est nécessaire pour\nsauvegarder les données. Continuer?"
    0x2242d8b8,		//"Aucune périphérique de stockage n'a été\ndétecté. Insérez le périphérique de stockage\nutilisé pour charger les données."
    0x2f3e4034,		//"Beim Speichern der Tarndaten\nauf das Speichergerät ist ein Fehler\naufgetreten. Noch einmal versuchen?"
    0x6a8f0fdd,		//"Bitte das Speichergerät anschließen,\num die\n Tarndaten zu speichern."
    0x24ff5247,		//"Camouflage data on storage device\nis broken."
    0x75a00047,		//"Carga satisfactoria de los datos de camuflaje\ndel dispositivo de almacenamiento."
    0xfb68d75e,		//"Cargando datos de camuflaje del\ndispositivo de almacenamiento."
    0x3cb62608,		//"Caricamento dati Mimetica da\nperiferica di memorizzazione completato."
    0x349e907b,		//"Caricamento dati Mimetica da\nperiferica di memorizzazione."
    0x08a1b370,		//"Caricamento fallito! Controllare la\nperiferica di memorizzazione e riprovare."
    0x09c88f58,		//"Caricamento fallito! Inserire la\nperiferica di memorizzazione usata\nper caricare i dati salvati."
    0xbc30de94,		//"Chargement réussi des données\nCamouflage sur le périphérique de stockage."
    0xcd026f45,		//"Checking storage device."
    0x0dbe10dd,		//"Checking storage device."
    0x0ff8ae84,		//"Checking storage device."
    0xc1f76008,		//"Checking storage device."
    0x32f65f03,		//"Collega una periferica di memorizzazione\nper salvare i dati Mimetica."
    0x00f4348d,		//"Comprobando dispositivo de almacenamiento."
    0x8aeb0012,		//"Comprobando dispositivo de almacenamiento."
    0x8c20f566,		//"Comprobando dispositivo de almacenamiento."
    0x8e664b3f,		//"Comprobando dispositivo de almacenamiento."
    0x3aa62129,		//"Conecta un dispositivo de almacenamiento."
    0xb01aa1f3,		//"Controllo periferica di memorizzazione fallito!\nServono almeno %dKB per salvare\ni dati. Continuare con il gioco?"
    0x51ac3ae6,		//"Controllo periferica di memorizzazione."
    0xfcf2670b,		//"Controllo periferica di memorizzazione."
    0xfeb4d952,		//"Controllo periferica di memorizzazione."
    0x23893e8d,		//"Creare nuovi dati salvati sulla\nperiferica di memorizzazione?\nSì o No"
    0xf3698f77,		//"Create new camouflage data save file on\nstorage device?"
    0x8e43883b,		//"Create new save data on\nstorage device?\nYes or No"
    0xf4f5c61a,		//"Créer de nouvelles données de\nsauvegarde sur le périphérique de stockage?\nOui ou Non"
    0x220e846d,		//"Créer une nouvelle sauvegarde pour les\ndonnées Camouflages sur le périphérique\nde stockage ?"
    0x2a22a67e,		//"Data on storage device is broken."
    0x580e7187,		//"Daten auf dem Speichergerät\nsind defekt."
    0xb34b381f,		//"Dati Mimetica su periferica di\n memorizzazione danneggiati."
    0xe24a846b,		//"Devi collegare una periferica di\nmemorizzazione per salvare il gioco."
    0x8fd73da6,		//"Diese Tarndaten auf das\nSpeichergerät speichern?"
    0x49c20d85,		//"Désolée, Snake. Il n'y a pas assez d'espace\nlibre sur ce périphérique de stockage."
    0xe43eb340,		//"Echec de la sauvegarde! Veuillez vérifier\nle périphérique de stockage et ré-essayer."
    0x287567d5,		//"Echec de la vérification du périphérique de stockage ! Un minimum de %dKB est\nnécessaire pour sauvegarder. Continuer?"
    0xd6f41e2c,		//"Echec du chargement! Insérez le périphérique de stockage utilisé pour charger les\ndonnées."
    0x95f77b9c,		//"Echec du chargement! Veuillez vérifier\nle périphérique de stockage et ré-essayer."
    0xc78b5f0b,		//"Echec du formatage! Veuillez vérifier\nle périphérique de stockage et ré-essayer."
    0x5d889205,		//"El dispositivo de almacenamiento no está\nformateado. ¿Quieres formatear\nel dispositivo de almacenamiento y guardar?"
    0x172d1854,		//"El dispositivo de almacenamiento no está formateado.\n¿Formatear dispositivo de almacenamiento y guardar\ndatos de camuflaje?"
    0x42d477c9,		//"El dispositivo de almacenamiento ya contiene estos\ndatos de camuflaje. ¿Quieres\nsobrescribir los datos?"
    0x8f0858b6,		//"Erreur de périphérique de stockage !\nVeuillez vérifier le disque\ndur et ré-essayer."
    0xd0b9876f,		//"Erreur lors de la vérification\ndu périphérique de stockage."
    0xede3430d,		//"Erreur lors du chargement des données\nCamouflage du périphérique de stockage."
    0xd7e95d95,		//"Error accessing storage device!"
    0xce23895d,		//"Error al cargar datos de camuflaje del\ndispositivo de almacenamiento."
    0xa9be3f28,		//"Error al comprobar el dispositivo de almacenamiento."
    0xce0725a0,		//"Error al detectar el dispositivo de almacenamiento."
    0xdc938906,		//"Error al detectar el dispositivo de almacenamiento."
    0x9514a7a3,		//"Error detecting the storage device."
    0xb57ed637,		//"Error detecting the storage device."
    0x15ef1c3b,		//"Error occurred checking storage device."
    0x4db85b94,		//"Error occurred loading Camouflage data from\nstorage device."
    0x6c99e588,		//"Errore durante caricamento dati Mimetica\nda periferica di memorizzazione."
    0x4e85255d,		//"Errore durante controllo periferica di memorizzazione."
    0x3f47002d,		//"Errore nel rilevamento della\nperiferica di memorizzazione."
    0xaa91c7ab,		//"Errore nel rilevamento della\nperiferica di memorizzazione."
    0x999ae9e6,		//"Errore nella periferica di memorizzazione.\nControlla la periferica di memorizzazione\ne riprova."
    0x57daa698,		//"Espace insuf. sur le périphérique de stockage.\nUn minimum de %dKB est nécessaire pour\nsauvegarder les données. Continuer?"
    0x12ef9304,		//"Espace libre insuff. sur le périphérique de stockage\n Un minimum de %dKB est néc.\npour sauv. les données Camouflage."
    0x49ae85f4,		//"Espace libre insuffisant sur le périphérique de stockage. Un minimum de %dKB est\nnécessaire pour sauvegarder les données."
    0xcc9cf0eb,		//"Espacio insuficiente en el dispositivo de almacenamiento.\nPara guardar datos hacen falta\nal menos %dKB libres. ¿Quieres jugar?"
    0xeabae06e,		//"Espacio insuficiente en el dispositivo de almacenamiento.\nPara guardar datos hacen falta\nal menos %dKB libres. ¿Quieres jugar?"
    0xc91869ed,		//"Espacio insuficiente en el dispositivo de almacenamiento. Necesitas %dKB para poder\nguardar datos de camuflaje."
    0x4195da36,		//"Fehler beim Erkennen des Speichergeräts."
    0xa2ea8f03,		//"Fehler beim Erkennen des Speichergeräts."
    0xe6c44634,		//"Fehler beim Laden der Tarndaten vom\nSpeichergerät."
    0x63ce74c1,		//"Fehler beim Prüfen des Speichergeräts."
    0xeb4bb5c5,		//"Fehler beim Überprüfen des Speichergeräts!\nBitte überprüfen Sie das Speichergerät\nund versuchen Sie es nochmal."
    0xd7b57220,		//"Format failed! Please check storage device and try again."
    0xa971a6d0,		//"Formatage de le périphérique de stockage\nen cours. Ne pas retirer la Memory Card\n(PS2), réinitialiser ou éteindre la console."
    0x1a83d52c,		//"Formatage en cours. Ne pas retirer le périphérique de stockage ou la manette, ni\nredémarrer ou éteindre la console."
    0xe795e5b3,		//"Formateando dispositivo de almacenamiento.\nNo extraigas el dispositivo de almacenamiento, ni\nreinicies o apagues la consola."
    0x904da7f0,		//"Formateando el dispositivo de almacenamiento.\nNo extraigas el dispositivo de almacenamiento o el\nmando ni resetees o apagues la consola."
    0xa4280469,		//"Formatiere Speichergerät. Speichergerät\nnicht entfernen und die Konsole\nnicht zurücksetzen oder ausschalten."
    0xc6a0aa24,		//"Formatieren fehlgeschlagen! Bitte überprüfen\nSie das Speichergerät und\nversuchen Sie es noch einmal."
    0x8d2864d5,		//"Formatiert Speichergerät. Speicher-\ngerät u. Controller nicht entfernen u.\nKonsole nicht zurückstellen o. abschalten."
    0x4707182a,		//"Formattazione fallita! Controllare\nla periferica di memorizzazione e riprovare."
    0xfa7ecb60,		//"Formattazione in corso per la periferica di memorizzazione\n.\nNon rimuovere la periferica di memorizzazione,\nnon resettare o spegnere la console."
    0x7e423cc5,		//"Formattazione periferica di memorizzazione. Non\nrimuovere periferica di memorizzazione e controller.\nNon resettare/spegnere la console."
    0x76ade753,		//"Formatting storage device.\nDo not remove storage device,\ncontroller, or reset/switch off the console."
    0x290c11f6,		//"Formatting storage device. Do not\nremove storage device, reset, or\nswitch off the console."
    0xc992c5f6,		//"Geladene Daten nicht gefunden.\nDerzeit gespeicherte Daten auf derm\nSpeichergerät überschreiben?"
    0x7a8a7e03,		//"Gespeicherte Daten sind defekt. Schließen Sie ein Speichergerät an, das zum Laden der\ngespeicherten Daten verwendet wurde."
    0xcd950c2f,		//"Guardando datos. No extraigas\nel dispositivo de almacenamiento o el mando\nni resetees o apagues la consola."
    0xb633a02a,		//"Guardando en el dispositivo de almacenamiento.\nNo extraigas el dispositivo de almacenamiento, ni\nreinicies o apagues la consola."
    0xcee54c88,		//"I dati salvati e caricati non sono stati\ntrovati. Sovrascrivere i dati salvati\nattuali sulla periferica di memorizzazione?"
    0x4f25638d,		//"I dati salvati sono danneggiati.\nInserire la periferica di memorizzazione\nusata per caricare i dati salvati."
    0x63e7dd8a,		//"I dati sulla periferica di memorizzazione\nsono danneggiati."
    0x5568ab95,		//"Inserta un dispositivo de almacenamiento\npara guardar los\ndatos de camuflaje."
    0x6bde7f44,		//"Insufficient free space on storage device.\nAt least %dKB of free space is required to\nsave data. Contiune with the game?"
    0xc01acd97,		//"Insufficient free space on storage device. \nAt least %dKB of free space\nis required to save Camouflage data."
    0x9bbdc7af,		//"Insufficient free space on storage device. At least %dKB of free space is\nrequired to save data."
    0x78d93d5b,		//"Kein Speichergerät gefunden."
    0xc7e1080e,		//"Kein Speichergerät gefunden.\n Schließen Sie\nein Speichergerät an, das zum\nLaden der Speicherdaten verwendet wurde."
    0x9527dfe3,		//"Kein Speichergerät gefunden.\nBitte Speichergerät anschließen.\nNoch einmal versuchen?"
    0xb18e3ef2,		//"Kein Speichergerät gefunden.\nFortfahren ohne zu speichern?"
    0x090f59de,		//"Kein Speichergerät gefunden.\nMindestens %d KB freier Speicherplatz nötig,\num Daten zu speichern. Spiel fortsetzen?"
    0x4ae7bbd9,		//"Kein Speichergerät gefunden.\nSpiel fortsetzen?"
    0xcb2a31cb,		//"Keine Daten auf dem Speichergerät vorhanden."
    0xa8371fe7,		//"Keine Daten auf dem Speichergerät vorhanden.\nSpiel fortsetzen ohne zu speichern?"
    0x6639b81d,		//"Keine Daten auf dem Speichergerät vorhanden.\nSpiel fortsetzen?"
    0x4452501c,		//"Keine geladenen Daten gefunden.\nSchließen Sie ein Speichergerät an, das zum\nLaden der Speicherdaten verwendet wurde."
    0xacefe0e5,		//"Keine Tarndaten auf dem Speichergerät\nvorhanden."
    0x256b8d05,		//"La periferica di memorizzazione\ncontiene già questi dati Mimetica.\nVuoi sovrascrivere i dati?"
    0xd885d6a4,		//"La periferica di memorizzazione\nnon contiene dati Mimetica."
    0xfdb5ff5d,		//"La periferica di memorizzazione non è \nformattata. Formattare la periferica di \nmemorizzazione e salvare?"
    0x6e77582e,		//"La periferica di memorizzazione non è formattata.\nVuoi formattare la periferica di memorizzazione\ne salvare i dati Mimetica?"
    0x9ef5fd5d,		//"Laden fehlgeschlagen! Schließen Sie ein\nSpeichergerät an, das zum Laden\n der\ngespeicherten Daten verwendet wurde."
    0x74d2f9cd,		//"Laden gescheitert. Bitte überprüfen Sie\ndas Speichergerät und versuchen\nSie es noch einmal."
    0x1e670678,		//"le périphérique de stockage a été retirée lors\nde la sauvegarde. Ré-insérez le périphérique de stockage. Annuler l'écrasement?"
    0xc5ebbe25,		//"le périphérique de stockage contient déjà\nces données Camouflage.\nSauvegarder par-dessus ?"
    0x274aa5d0,		//"Le périphérique de stockage n'est pas\nformaté. Formater le périphérique de stockage et sauvegarder?"
    0xc9c40027,		//"Le périphérique de stockage n'est pas formaté.\nFormater le périphérique de stockage puis\nsauvegarder les données Camouflage ?"
    0xc9ec73ec,		//"Les données Camouflage sur le\npériphérique de stockage sont corrompues."
    0xa3fccd36,		//"Les données sauvegardées chargées n'ont\npas été trouvées. Ecraser sur le périphérique de stockage les données sauv. présentes?"
    0x41e97066,		//"Les données sauvegardées sont\ncorrompues. Insérez le périphérique de stockage utilisé pour charger les données."
    0xb2731b1f,		//"Les données sur le périphérique de stockage\nsont corrompues."
    0xe90fcbc4,		//"Lo siento, Snake. Parece que no queda espacio\nlibre en el dispositivo de almacenamiento."
    0xda05d36a,		//"Load failed! Check storage device\nand please try again."
    0x83ba9b2d,		//"Load failed! Connect storage device\nthat was used to load the saved\ndata."
    0x3a4dbe9b,		//"Loading Camouflage data from\nstorage device."
    0x7fd9f0d6,		//"Loading Camouflage data from storage device successfully completed."
    0xb130c978,		//"Los datos de camuflaje del\ndispositivo de almacenamiento están dañados."
    0x59e22308,		//"Los datos del dispositivo de almacenamiento están dañados."
    0xd546d0dd,		//"Los datos guardados están dañados.\nConecta el dispositivo de almacenamiento desde\nel que cargaste los datos guardados."
    0x0fc563c4,		//"Nessun dato caricato trovato.\nInserire la periferica di memorizzazione\nusata per caricare i dati salvati."
    0x4239cfa8,		//"Nessun dato presente sulla\nperiferica di memorizzazione."
    0xbd8b7bbb,		//"Nessun dato presente sulla periferica\ndi memorizzazione. Continuare con il gioco?"
    0xf07dab0c,		//"Nessun dato presente sulla periferica\ndi memorizzazione. Continuare senza salvare?"
    0x18502c68,		//"Nessuna periferica di memorizzazione trovata."
    0x83a56edb,		//"Nessuna periferica di memorizzazione trovata.\nCollega la periferica di memorizzazione usata\nper caricare i dati salvati."
    0xf5efc328,		//"Nessuna periferica di memorizzazione trovata.\nContinuare con il gioco?"
    0x99330d4a,		//"Nessuna periferica di memorizzazione trovata.\nContinuare senza salvare?"
    0x81a483a0,		//"Neue Datei auf dem Speichergerät\nerstellen?\nJa oder Nein"
    0xffa8a482,		//"Neue Speicherdatei für Tarndaten\nzum Speichern auf dem Speichergerät\nerstellen?"
    0x1320d0ff,		//"No Camouflage data present\non storage device."
    0x2b955315,		//"No Camouflage data present on \nstorage device."
    0x8820bfd3,		//"No data present on storage device."
    0x8b17cf4d,		//"No data present on storage device.\nContinue with the game?"
    0x546a935d,		//"No data present on storage device.\nContinue without saving?"
    0xf10a3d7d,		//"No hay datos de camuflaje\nen el dispositivo de almacenamiento."
    0x070355db,		//"No hay datos en el dispositivo de almacenamiento."
    0xa43c291e,		//"No hay datos en el dispositivo de almacenamiento.\n¿Quieres continuar jugando?"
    0x814e01d2,		//"No hay datos en el dispositivo de almacenamiento.\n¿Quieres continuar sin guardar?"
    0xdfb88ca3,		//"No hay ningún dispositivo de almacenamiento conectado."
    0x3db1b465,		//"No hay ningún dispositivo de almacenamiento conectado.\n¿Quieres continuar jugando?"
    0xcf45c00a,		//"No hay ningún dispositivo de almacenamiento conectado.\n¿Quieres continuar sin guardar?"
    0xf79c34d2,		//"No hay suficiente espacio en el disco\nduro. Para guardar datos\nnecesitas, al menos, %dKB libres."
    0x28c3eb49,		//"No loaded data found. storage device \nthat was used\nto load the saved data."
    0x6e3ac750,		//"No se encontró ningún disco\nduro. Debes conectar uno.\n¿Quieres intentarlo de nuevo?"
    0x03889dcf,		//"No se encuentran los datos cargados. ¿Quieres sobreescribir los datos\npresentes en el dispositivo de almacenamiento?"
    0xf35628d4,		//"No se ha encontrado\n ningún\ndispositivo de almacenamiento."
    0x59874ee2,		//"No se ha encontrado ningún dispositivo de almacenamiento.\nConecta el dispositivo de almacenamiento desde el que\ncargaste los datos guardados."
    0x10f39024,		//"No se han encontrado datos cargados.\nConecta el dispositivo de almacenamiento desde\nel que cargaste los datos guardados."
    0xc1aed69e,		//"No storage device detected."
    0x1ae50052,		//"No storage device found.\nContinue with the game?"
    0xd9f36c67,		//"No storage device found.\nContinue without saving?"
    0x44f01b91,		//"No storage device found. At least %dKB\nof free space is required to save data.\nContinue with the game?"
    0x7ecaf75a,		//"No storage device found. Connect the \nstorage device that was used\nto load the saved data."
    0x8657f566,		//"No storage device was found."
    0x41b48956,		//"Ocurrió un error al guardar los datos de\ncamuflaje en el dispositivo de almacenamiento.\n¿Quieres intentarlo de nuevo?"
    0xa94a36c6,		//"Para guardar datos de camuflaje\nse necesitan al menos %dKB espacio libre\nen el dispositivo de almacenamiento. ¿Quieres continuar?"
    0x7c26f4b2,		//"Para guardar la partida, necesitas un\ndispositivo de almacenamiento."
    0x36f8443a,		//"Pas de données Camouflage présentes\nsur le périphérique de stockage."
    0x66d3ebcd,		//"Pas de données présentes sur\nle périphérique de stockage."
    0x9786c774,		//"Pas de données présentes sur le périphérique de stockage. Continuer avec la partie?"
    0xd47e69c7,		//"Pas de données présentes sur le périphérique de stockage. Continuer sans sauvegarder?"
    0x55a6e434,		//"Per salvare i dati Mimetica servono\nalmeno %dKB di spazio libero sulla\nperiferica di memorizzazione. Continuare?"
    0x0da471aa,		//"Periferica di memorizzazione non trovata.\nServono almeno %dKB per salvare\ni dati. Continuare con il gioco?"
    0xea763b61,		//"Periferica di memorizzazione non trovata. \nCollega una periferica di memorizzazione.\nVuoi provare nuovamente a salvare?"
    0xb1645c15,		//"Periferica di memorizzazione rimossa durante\nil salvataggio. Ricollegare la periferica\ndi memorizzazione. Annullare la sovrascrittura?"
    0xad8ac2cd,		//"Please connect storage device to save\ncamouflage data."
    0x10612a65,		//"périphérique de stockage non trouvé.\nConnectez un périphérique de stockage.\nSauvegarder de nouveau ?"
    0xa36c95d5,		//"Quindi collega la tua periferica di memorizzazione."
    0xf210692e,		//"Reading Camouflage data from storage device."
    0xfef33d34,		//"Retrieve Camouflage list from \nstorage device. Press ."
    0xf0e53e1a,		//"Retrieve Camouflage list from storage device."
    0x6b22b805,		//"Retrieving downloaded Camouflage list\nfrom storage device."
    0xcb5e5f23,		//"Retrouver les données Camouflage\nsur le périphérique de stockage."
    0xdc1ba6be,		//"Salvataggio fallito! Controllare\nla periferica di memorizzazione e riprovare."
    0x346dab9e,		//"Salvataggio in corso sulla periferica di memorizzazione. \nNon rimuovere la periferica di memorizzazione,\nnon resettare o spegnere la console."
    0x84ffffab,		//"Salvataggio... Non rimuovere la\nperiferica di memorizzazione né il controller\ne non resettare/spegnere la console."
    0x53e31119,		//"Sauvegarde des données. Ne pas retirer le périphérique de stockage ou la manette, ni\nredémarrer ou éteindre la console."
    0x5d77b7b1,		//"Sauvegarde du périphérique de stockage\nen cours. Ne pas déconnecter le disque\ndur, réinitialiser ou éteindre la console."
    0x83625f73,		//"Sauvegarder les données Camouflage\nsur le périphérique de stockage ?"
    0x9832f239,		//"Save failed! Check storage devicee\nand please try again."
    0x7f5c6c6c,		//"Save this camouflage data to \nstorage device?"
    0xe3d73d75,		//"Saved data is broken. Connect \nstorage device that was used\nto load the saved data."
    0x7675cbb2,		//"Saved data that was loaded is not found.\nOverwrite the present saved data on\nthe storage device?"
    0x258854c3,		//"Saving data.\nDo not remove storage device,\ncontroller, or reset/switch off the console."
    0x474fa529,		//"Saving to storage device. Do not\nremove storage device, reset, or\nswitch off the console."
    0xfcbb2b56,		//"Schließen Sie also ein Speichergerät."
    0xc9f5c968,		//"Scusa, Snake. A quanto pare, non c'è\nabbastanza spazio sulla periferica di\nmemorizzazione."
    0xecb49083,		//"Se ha extraído el dispositivo de almacenamiento mientras\nse guardaba la partida. Conéctalo de\nnuevo. ¿Cancelar sobrescritura?"
    0xe46caecf,		//"Selecciona los datos de\ncamuflaje para guardar\nen el dispositivo de almacenamiento."
    0x2ba89bb5,		//"Selecciona un dispositivo de almacenamiento para\nguardar\n los datos de camuflaje."
    0x84a5fc5b,		//"Select camouflage data to save to\nstorage device."
    0x65fa0e9b,		//"Select storage device to save\ncamouflage data."
    0x1f0bb047,		//"Seleziona i dati Mimetica da \nsalvare\nsulla periferica di memorizzazione."
    0x4255c8fd,		//"Seleziona la periferica di memorizzazione\ndove vuoi salvare i dati\n Mimetica."
    0x32cc893f,		//"Si è verificato un errore nel salvataggio \ndei\ndati Mimetica sulla periferica di memorizzazione.\nVuoi provare nuovamente a salvare?"
    0x1ceaaa40,		//"Snake, es ist kein Speichergerät angeschlossen."
    0xa80c23bf,		//"Snake, il n'y a pas de périphérique\nde stockage connecté."
    0xc2df77ed,		//"Snake, no hay ningún dispositivo de almacenamiento\nconectado."
    0x6f47fb0e,		//"Snake, non c'è alcuna periferica\ndi memorizzazione collegata!"
    0x5f88d871,		//"Snake, there's no storage device connected!"
    0x292bedef,		//"So connect your storage device."
    0x4fecc712,		//"Sorry, Snake. There doesn't seem to be any\nfree space."
    0xe23220b1,		//"Spazio insufficiente sulla periferica di memorizzazione.\nServono almeno %dKB per salvare i dati.\nContinuare con il gioco?"
    0xfa49f799,		//"Spazio libero insufficiente sulla\nperiferica di memorizzazione. \nServono almeno %dKB di spazio\nlibero per salvare i dati Mimetica."
    0x52211bda,		//"Spazio libero insufficiente sulla periferica\ndi memorizzazione. Sono richiesti almeno \n%dKB di spazio libero per salvare i dati."
    0x0f7cac4c,		//"Speichere auf das Speichergerät. Speichergerät\nnicht entfernen und die Konsole\nnicht zurücksetzen oder ausschalten."
    0x1a24429e,		//"Speichergerät enthält bereits\ndiese Tarndaten.\nDaten überschreiben?"
    0xc93da4aa,		//"Speichergerät ist nicht formatiert.\nSpeichergerät formatieren und\ndie Tarndaten speichern?"
    0x32981872,		//"Speichergerät ist nicht formatiert.\nSpeichergerät formatieren und\nspeichern?"
    0x0afd6477,		//"Speichergerät prüfen."
    0xf1f76c4c,		//"Speichergerät prüfen."
    0xf3b1d215,		//"Speichergerät prüfen."
    0x6d684b99,		//"Speichergerät wurde während des\nSpeicherns entfernt. Speichergerät\n wieder anschließen. Überschreiben abbrechen?"
    0x3da31c46,		//"Speichern fehlgeschlagen! Bitte überprüfen\nSie das Speichergerät und\nversuchen Sie es noch einmal."
    0xcae01fc2,		//"Speichert Daten. Speichergerät und\nController nicht entfernen und die Konsole\nnicht zurückstellen oder abschalten."
    0x7cd12acb,		//"Storage device already \ncontains this\n camouflage data. \nOverwrite data?"
    0xe7e143d5,		//"Storage device being used has been changed."
    0xbc9a0b27,		//"Storage device error! Please check\nstorage device and try again."
    0xd904a049,		//"Storage device failed! At least\n%dKB of free space is required to save data.\nContinue with the game?"
    0xba50e82e,		//"Storage device is not formatted.\nFormat storage device and save\ncamouflage data?"
    0x67aad628,		//"Storage device is not formatted.\nFormat storage device and save?"
    0x50ab62ce,		//"Storage device not found.\nPlease connect hard disk drive.\nTry saving again?"
    0x95897a6f,		//"Storage device was removed during\nsaving. Re-connect storage device.\nCancel overwriting?"
    0x8875ee3d,		//"Sélectionner le périphérique de stockage\npour sauvegarder les données\nCamouflage."
    0x873459d7,		//"Sélectionner les données Camouflage\nà sauvegarder sur le périphérique de stockage."
    0x8527d634,		//"Tarndaten auf dem Speichergerät\nsind defekt."
    0xa5384bb2,		//"Tarndaten erfolgreich vom Speichergerät\ngeladen."
    0x9ffd7986,		//"Tarndaten vom Speichergerät laden."
    0x7e637134,		//"Tu as besoin d'un périphérique de\nstockage pour sauvegarder ta partie."
    0xec44f2e8,		//"Tut mir leid, Snake. Auf dem Speichergerät\nscheint kein freier Speicherplatz zu sein."
    0x01783889,		//"Um das Spiel zu speichern, benötigen Sie ein\nSpeichergerät."
    0x30b6ebec,		//"Un min. de %dKB d'espace libre est néc.\nsur le périphérique de stockage pour sauv.\nles données Camouflage. Continuer?"
    0x24c30b73,		//"Une erreur est survenue lors de la\nsauvegarde sur le périphérique de stockage.\nSauvegarder de nouveau ?"
    0xd0d9960e,		//"Ungenüg. Speicherplatz auf dem Speichergerät\n.\nZ. Speichern der Tarndaten werden\nmind. %d KB freier Speicherplatz benötigt."
    0xcd02a5f2,		//"Ungenügend freier Speicherplatz auf dem Speichergerät. Zum Speichern werden\nmind. %d KB freier Speicherplatz benötigt."
    0x00cf2f38,		//"Ungenügend Speicherplatz auf dem Speichergerät. Zum Speichern sind mind. %d KB\n freier Speicherplatz nötig. Spiel fortsetzen?"
    0x9eb911c3,		//"Veuillez connecter un périphérique de stockage\npour sauvegarder les données\nCamouflage."
    0x1a1f45a1,		//"Veuillez connecter un périphérique de stockage."
    0xb374b97a,		//"Veuillez connecter un périphérique de stockage."
    0x9787a258,		//"Vuoi creare un nuovo file di\nsalvataggio per i dati Mimetica\nsulla periferica di memorizzazione?"
    0x39ba15be,		//"Vuoi salvare questi dati Mimetica\nsulla periferica di memorizzazione?"
    0x2571a9b0,		//"Vérification du périphérique de stockage."
    0x273717e9,		//"Vérification du périphérique de stockage."
    0x4cc73cb6,		//"Vérification du périphérique de stockage."
    0x8a81578e,		//"Wähle das Speichergerät,\num die Tarndaten zu speichern."
    0x67222e03,		//"Wählen Sie die Tarndaten aus,\num sie auf das Speichergerät\nzu speichern."
    0x5360fe03,		//"You need to connect the storage device\n to save\n the game."
    0x11f0f9f1,		//"Zum Speichern der Tarndaten wird mind.\n%d KB Speicherplatz auf dem Speicherplatz\nbenötigt. Fortfahren?"
    0xacd4f3b5,		//"¡Error al cargar! Conecta el disco\nduro desde ell que cargaste\nlos datos guardados."
    0x2331a700,		//"¡Error al cargar! Por favor\ncomprueba el dispositivo de almacenamiento\ne inténtalo de nuevo."
    0x6a4db079,		//"¡Error al comprobar el dispositivo de almacenamiento!\nPara guardar datos hacen falta\nal menos %dKB libres. ¿Quieres jugar?"
    0xc6089e54,		//"¡Error al formatear! Por favor\ncomprueba el dispositivo de almacenamiento\ne inténtalo de nuevo."
    0xea0ff320,		//"¡Error al guardar! Por favor\ncomprueba el dispositivo de almacenamiento\ne inténtalo de nuevo."
    0x6a14e75b,		//"¡Error en el dispositivo de almacenamiento! Comprube el\ndispositivo de almacenamiento e inténtalo de nuevo."
    0xfa3fc73f,		//"¿Crear un nuevo archivo\nde datos de camuflaje en\nel dispositivo de almacenamiento?"
    0x0c8381e4,		//"¿Guardar estos datos de\ncamuflaje en el dispositivo de almacenamiento?"
    0x6545dbd9,		//"¿Quieres crear un nuevo archivo\npara guardar información en el\nel dispositivo de almacenamiento? Sí o No"
    0xbd2bace9,		//"Überprüfen des Speichergeräts fehlgeschlagen. Mind. %d KB freier Speicherplatz nötig,\num Daten zu speichern. Spiel fortsetzen?"
    0x048691fe,		//"※これにあたるメッセージは今回表示しない"
    0x0ac36d1a,		//"※これにあたるメッセージは今回表示しない"
    0x127febe5,		//"※これにあたるメッセージは今回表示しない"
    0x1404210a,		//"※これにあたるメッセージは今回表示しない"
    0x15ce2152,		//"※これにあたるメッセージは今回表示しない"
    0x1ae4cd7f,		//"※これにあたるメッセージは今回表示しない"
    0x254a919f,		//"※これにあたるメッセージは今回表示しない"
    0x2c50b1b2,		//"※これにあたるメッセージは今回表示しない"
    0x37136c29,		//"※これにあたるメッセージは今回表示しない"
    0x3abb2d4d,		//"※これにあたるメッセージは今回表示しない"
    0x3d0903e4,		//"※これにあたるメッセージは今回表示しない"
    0x3eb52dfc,		//"※これにあたるメッセージは今回表示しない"
    0x41230e48,		//"※これにあたるメッセージは今回表示しない"
    0x46bf7861,		//"※これにあたるメッセージは今回表示しない"
    0x47f0a2a4,		//"※これにあたるメッセージは今回表示しない"
    0x577b3ca9,		//"※これにあたるメッセージは今回表示しない"
    0x64cb2d69,		//"※これにあたるメッセージは今回表示しない"
    0x745234c6,		//"※これにあたるメッセージは今回表示しない"
    0x84cd77df,		//"※これにあたるメッセージは今回表示しない"
    0x94674e81,		//"※これにあたるメッセージは今回表示しない"
    0x9bb5f429,		//"※これにあたるメッセージは今回表示しない"
    0x9dd763fb,		//"※これにあたるメッセージは今回表示しない"
    0xd006f3df,		//"※これにあたるメッセージは今回表示しない"
    0xd5b5cc7b,		//"※これにあたるメッセージは今回表示しない"
    0xec9a00fb,		//"※これにあたるメッセージは今回表示しない"
    0xf17244d0,		//"※これにあたるメッセージは今回表示しない"
    0xf7927887,		//"※これにあたるメッセージは今回表示しない"
    0x91767ceb,		//"ストレージからのデー夕のロードが\n正常に終了しました。"
    0xa01441b9,		//"ストレージからのデー夕のロード中に\nエラーが発生しました。"
    0x4ea5f6d8,		//"ストレージからデー夕をロードしています。"
    0xf141bf37,		//"ストレージがフォーマットされていま\nせん。ストレージをフォーマット\nしてセーブします。よろしいですか？"
    0x3d518472,		//"ストレージが抜き差しされました。\n元のストレージを接続してください。\n上書きセーブを中止しますか？"
    0x9e42826b,		//"ストレージが見つかりません。"
    0xba2f14f8,		//"ストレージが見つかりません。\nこのままゲームを開始しますか？"
    0x7f9f3321,		//"ストレージが見つかりません。\nセーブせずに終了してよろしいですか？"
    0x6ff18cdc,		//"ストレージが見つかりません。\nセーブデータの保存には、%dキロバイト以上の\n空き容量が必要です。このままゲームを開始しますか？"
    0x5b1a2a5f,		//"ストレージが見つかりません。\nセーブデー夕をロードした際に使用した\nストレージをセットしてください。"
    0x645f86d3,		//"ストレージにデータが見つかりません。"
    0x151fc749,		//"ストレージにデータが見つかりません。\nこのままゲームを開始しますか？"
    0xe83b9069,		//"ストレージにデータが見つかりません。\nセーブせずに終了してよろしいですか？"
    0x2ca7d835,		//"ストレージにデータをセーブしていま\nす。本体の\n電源OFFやリセットは\n行わないでください。"
    0xbb6ef73b,		//"ストレージにデータを上書きセーブ\nします。よろしいですか？"
    0xaa890002,		//"ストレージにデータを新規セーブします。\nよろしいですか？"
    0x441b7708,		//"ストレージに空き容量が足りません。\nセーブデー夕の保存には、%dキロバイト以上の\n空き容量が必要です。このままゲームを開始しますか？"
    0xed2e9b3e,		//"ストレージのチェック中にエラーが\n発生しました。"
    0xc9a93ec8,		//"ストレージのデータが壊れています。"
    0x6f4bdf2c,		//"ストレージのフォーマット中に\nエラーが発生しました。"
    0x95685399,		//"ストレージの検出でエラーが発生しました。\nセーブデータの保存には、%dキロバイト以上の\n空き容量が必要です。このままゲームを開始しますか？"
    0x20028b92,		//"ストレージの空き容量が足りません。\nセーブデー夕の保存には、%dキロバイト以上の\n空き容量が必要です。"
    0xf19dc507,		//"ストレージへのデータのセーブが\n正常に終了しました。"
    0x95ecfd3c,		//"ストレージへのデータのセーブ中に\nエラーが発生しました。"
    0xf81afb07,		//"ストレージをセットしてください。"
    0x55e5d5d6,		//"ストレージをチェックしています。"
    0x8341785b,		//"ストレージをフォーマットしています。\nストレージの抜き差し、本体の電源OFFや\nリセットは行わないでください。"
    0x31050144,		//"セーブデータが壊れています。\nセーブデータをロードした際に使用した\nストレージをセットしてください。"
    0xc4c54f5c,		//"セーブデー夕のロード中にエラーが発生しま\nした。セーブデー夕をロードした際に使用した\nストレージをセットしてください。"
    0x59135e07,		//"ロードされたセーブデータが見つかりません。\nストレージ上に現在のセーブデータを\n上書きしてよろしいですか？"
    0x12c10d87,		//"ロードしたデータが見つかりません。\nセーブデータをロードした際に使用した\nストレージをセットしてください。"
    0x7f66dba5,		//"別のストレージをセットしてください。"
};


constexpr CaptionOverride kMGS2ForcePs2Corrections[] =
{
    0x18D96931, "Checking MEMORY CARD (PS2)", // "Checking hard disk drive"
    0x524558F1, "Comprobando MEMORY CARD (PS2)", // "Comprobando disco duro"
    0xBF173BA0, "Controllo MEMORY CARD (PS2).", // "Controllo disco fisso."
    0x7C955257, "Die MEMORY CARD (PS2) enthält keine Daten.", // "Die Festplatte enthält keine Daten."
    0x02830781, "MEMORY CARD (PS2)", // "Disco duro"
    0x12082FD8, "MEMORY CARD (PS2)", // "Disco fisso"
    0xE5498091, "MEMORY CARD (PS2)", // "Disque dur"
    0xAD2B5773, "MEMORY CARD (PS2)", // "Festplatte"
    0xFB233A6F, "MEMORY CARD (PS2) wird überprüft.", // "Festplatte wird überprüft."
    0x646C0AE2, "メモリーカード(PS2)", // "HDD"
    0xCED90214, "メモリーカード(PS2)にデータがありません。", // "HDDにデータがありません。"
    0xD9A378E9, "メモリーカード(PS2)の確認中です", // "HDDの確認中です"
    0x9E22D110, "MEMORY CARD (PS2)", // "Hard disk drive"
    0x455F1B84, "La MEMORY CARD (PS2) ne contient pas de données.", // "Le disque dur ne contient pas de données."
    0x81D90724, "Nessun dato salvato presente sulla MEMORY CARD (PS2).", // "Nessun dato salvato presente sull'unità disco fisso."
    0xD01BD49C, "No data present on MEMORY CARD (PS2).", // "No data present on hard disk drive."
    0x61FFCFA5, "No hay datos en la MEMORY CARD (PS2).", // "No hay datos en el disco duro."
    0x25D5E9CD, "Vérification de la MEMORY CARD (PS2)", // "Vérification du disque dur"
};

constexpr CaptionOverride kMGS3ForcePs2Corrections[] =
{
    0x4AF67E67, "Checking memory card (PS2).", // "CHECKING hard disk drive"
    0xF80F07AF, "Checking memory card (PS2).", // "CHECKING hard drive"
    0xCD026F45, "Checking memory card (PS2).", // "Checking storage device"
    0x6CA23831, "Comprobando Memory Card (PS2).", // "COMPROBANDO hard drive"
    0xE173B3B2, "Comprobando Memory Card (PS2).", // "Comprobando dispositivo de almacenamiento."
    0xAEB666D6, "Controllo memory card (PS2).", // "CONTROLLO hard disk"
    0xE90121F9, "Controllo memory card (PS2).", // "CONTROLLO hard drive"
    0xA7D29A66, "Controllo memory card (PS2).", // "Controllo periferica di memorizzazione."
    0xBD0D857D, "Memory Card (PS2)", // "Disp. de almacenamiento"
    0x9C930788, "Memory Card (PS2)", // "HARD DISK"
    0xF43A21CC, "Memory Card (PS2)", // "HARD DISK DRIVE"
    0x5E8E58A3, "Memory Card (PS2)", // "HARD DRIVE"
    0x7DBFAE4C, "Keine Daten auf der Memory Card (PS2) vorhanden.", // "Keine Daten auf dem Speichergerät vorhanden."
    0x455F1B84, "Pas de données présentes sur la Memory Card (PS2).", // "Le disque dur ne contient pas de données."
    0x6BF9B773, "Nessun dato presente sulla memory card (PS2).", // "Nessun dato presente sulla periferica di memorizzazione."
    0x64DE27EE, "No data present on memory card (PS2).", // "No data present on storage device."
    0x8E5C562F, "No hay datos en la Memory Card (PS2).", // "No hay datos en el dispositivo de almacenamiento."
    0xED834D91, "Memory card (PS2)", // "Periferica di memorizzazione"
    0x9453A735, "Memory Card (PS2)", // "Périphérique de stockage"
    0x1E25E0AB, "Memory Card (PS2)", // "Speichergerät"
    0xC6048F06, "Memory Card (PS2) wird überprüft.", // "Speichergerät wird überprüft."
    0xE2C49FA3, "Memory Card (PS2)", // "Storage Device"
    0x0F0ED6F1, "Vérification de la Memory Card (PS2).", // "VERIFICATION hard drive"
    0xEE1CBC44, "Vérification de la Memory Card (PS2).", // "Vérification du périphérique de stockage."
    0xFD93EA20, "メモリーカード(PS2)にデータがありません。", // "データ保存機器にデータがありません。"
    0xE8CE489A, "メモリーカード(PS2)をチェックしています。", // "データ保存機器の確認中です。"
};


#pragma endregion mgs3_ps2_memory_card_strings

