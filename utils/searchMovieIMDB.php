<?php
    /* This needs imdbphp (http://projects.izzysoft.de/trac/imdbphp) to work */
    require_once('imdbphp/imdb.class.php');

    /* set the content type to be XML, so that the browser will recognise it as XML */
    header ('content-type: application/xml; charset=utf-8');
    
    /* function of creation of the document */
    function
    createDocument ()
    {
        $document = new DomDocument ();
        $document->encoding = "utf-8";

        return $document;
    }

    /* function generating the results tag */
    function generateResultsTag ($keywords, $document)
    {
        $resultsTag = $document->CreateElement ('results');

        $forAttr = $document->CreateAttribute ('for');
        $textForAttr = $document->CreateTextNode ($keywords);
        $forAttr->appendChild ($textForAttr);

        $opSearchAttr = $document->CreateAttribute ('xmlns:opensearch');
        $textOpSearchAttr =
        $document->CreateTextNode ('http://a9.com/-/spec/opensearch/1.1/');
        $opSearchAttr->appendChild ($textOpSearchAttr);

        $resultsTag->appendChild ($forAttr);
        $resultsTag->appendChild ($opSearchAttr);

        return $resultsTag;
    } 

    /* generate main xml tag */
    function generateOpenSearchQueryTag ($keywords, $document)
    {
        $queryTag = $document->CreateElement ('opensearch:Query');
        $searchKeyAttr = $document->CreateAttribute ('searchTerms');
        $searchKeyAttrText = $document->CreateTextNode ($keywords);
        $searchKeyAttr->appendChild ($searchKeyAttrText);

        $queryTag->appendChild ($searchKeyAttr);

        return $queryTag;
    }

    /* function generating the total results tag */
    function generateOpenSearchTotResultsTag ($totalRes, $document)
    {
        $totalResults = $document->CreateElement ('opensearch:totalResults');
        $totalResultsText = $document->CreateTextNode ($totalRes);
        $totalResults->appendChild ($totalResultsText);

        return $totalResults;
    }

    /* function generating no search results xml */
    function generate_no_search_results ($keywords)
    {
        $searchResultsDocument = createDocument ();
        $resultsTag = generateResultsTag ($keywords, $searchResultsDocument);
        $opensearchQueryTag = generateOpenSearchQueryTag ($keywords, $searchResultsDocument);
        $openSearchTotResultTag = generateOpenSearchTotResultsTag ('0', $searchResultsDocument);

        $moviematchestag = $searchResultsDocument->CreateElement ('moviematches');
        $movietag = $searchResultsDocument->CreateElement ('movie');
        $content = $searchResultsDocument->CreateTextNode (utf8_encode ('Your query didn\'t return any results.'));
        $movietag->appendChild ($content);
        $moviematchestag->appendChild ($movietag);

        $resultsTag->appendChild ($opensearchQueryTag);
        $resultsTag->appendChild ($openSearchTotResultTag);
        $resultsTag->appendChild ($moviematchestag);

        $searchResultsDocument->appendChild ($resultsTag);

        return $searchResultsDocument->saveXML ();
    }

    function generatePersonTag($document, $person, $job)
    {
        $person_tag = $document->CreateElement ('person');
        $job_tag = $document->CreateAttribute ('job');
        $textForAttr = $document->CreateTextNode ($job);
        $job_tag->appendChild ($textForAttr);
        $name = $document->createElement ('name', $person['name']);
        $role = $document->createElement ('role', $person['role']);
        $url = $document->createElement ('url', 'http://www.imdb.com/name/'.$person['imdb']);
            
        $person_tag->appendChild ($job_tag);
        $person_tag->appendChild ($name);
        $person_tag->appendChild ($role);
        $person_tag->appendChild ($url);
        
        return $person_tag;
    }

    /* generate the movie tag with the information of the movie */
    function generate_movietag ($result, $document)
    {
        $movietag = $document->CreateElement ('movie');
        
        $imdbid = $result->imdbid ();

        $url = 'http://www.imdb.com/title/'.$imdbid;
        $urltag = $document->CreateElement ('url', $url);
        $idtag = $document->CreateElement ('id', $imdbid);
        $type = $document->CreateElement ('type', 'movie');
        
        /* title attribute */
        $title = $document->CreateElement ('title', $result->title ());
        $t_attr = $document->CreateAttribute('lower');
        $textForTattr = $document->CreateTextNode(strtolower($title->nodeValue));
        $t_attr->appendChild($textForTattr);
        $title->appendChild($t_attr);

        /* alternative title attribute */
        foreach ($result->alsoknow () as $alt_title)
        {  
            $alternative_title = $document->CreateElement('alternative_title', $alt_title['title']);
            $at_attr = $document->CreateAttribute('lower');
            $textForAtAttr = $document->CreateTextNode(strtolower($alternative_title->nodeValue));
            $at_attr->appendChild($textForAtAttr);
            $alternative_title->appendChild($at_attr);
            $movietag->appendChild ($alternative_title);
        }
        
        /* check for people */
        $people = $document->CreateElement ('people', '');

        /* director of movie */
        foreach ($result->director () as $director)
        {
            $person = generatePersonTag($document, $director, 'director');
            $people->appendChild ($person);
        }

        /* actors information */
        $i = 1;
        foreach ($result->cast () as $actor)
        {
            $person = generatePersonTag($document, $actor, 'actor');
            $people->appendChild ($person);
            /* HACK return only the first five actors */
            if (++$i > 5) break;
        }

        /* categories information */
        $categories = $document->CreateElement ('categories', '');
        foreach ($result->genres() as $genre)
        {
            $category = $document->CreateElement ('category');
            $category_url = $document->CreateElement ('url', 'http://www.imdb.com/Sections/Genres/'.$genre);
            $category_name = $document->CreateElement ('name', $genre);
            $category->appendChild ($category_name);
            $category->appendChild ($category_url);
            $categories->appendChild ($category);
        }

        /* homepage information */
        $sites = $result->officialSites();
        if (count($sites) > 0)
            $homepage = $document->CreateElement ('homepage', $sites[0]['url']);
        else
            $homepage = $document->CreateElement ('homepage', '');
        
        /* description information */
        $plots = $result->plot();
        if (count($plots) > 0)
        {
            $plot = trim(substr($plots[0], 0, -strlen(stristr($plots[0], '<i>'))));
            $short_overview = $document->CreateElement ('short_overview', $plot);
        } else {
           $short_overview = $document->CreateElement ('short_overview', $result->plotoutline());
        }

        /* production countries */
        $countries = $document->CreateElement ('countries', '');
        foreach($result->country() as $country)
        {
            $country_tag = $document->CreateElement ('country');
            $country_sname = $document->CreateElement ('short_name', $country);
            $country_tag->appendChild($country_sname);
            $countries->appendChild($country_tag);
        }
         
        /* runtime information */
        $runtime = $document->CreateElement ('runtime', $result->runtime());
    
        /* rating information */
        $rating = $document->CreateElement ('rating', $result->rating ());

        /* release information */
        $release = $document->CreateElement ('release', $result->year().'-01-01');

        $movietag->appendChild ($rating);
        $movietag->appendChild ($title);
        $movietag->appendChild ($type);
        $movietag->appendChild ($idtag);
        $movietag->appendChild ($urltag);
        $movietag->appendChild ($short_overview);
        $movietag->appendChild ($release);
        $movietag->appendChild ($runtime);
        $movietag->appendChild ($countries);
        $movietag->appendChild ($homepage);
        $movietag->appendChild ($people);
        $movietag->appendChild ($categories);

        $document->appendChild ($movietag);

        return $movietag;
    }

    /* function generating the main movie tag */
    function generate_movies_tags ($results, $document, $limit = 0)
    {
        $moviematchestag = $document->CreateElement ('moviematches');
        
        $i = 1;
        foreach ($results as $res)
        {
            $movietag = generate_movietag ($res, $document);
            $moviematchestag->appendChild ($movietag);
            if ($limit > 0)
                if (++$i > $limit) break;
        }
        
        return $moviematchestag;
    }

    /* function generating the searche results tag */
    function generate_search_results ($keywords, $results, $limit = 0)
    {
        $searchResultsDocument = createDocument ();
        $resultsTag = generateResultsTag ($keywords, $searchResultsDocument);
        
        $opensearchQueryTag = generateOpenSearchQueryTag ($keywords, $searchResultsDocument);
        if ($limit > 0)
            $rescount = $limit;
        else
            $rescount = count($results);
        $openSearchTotResultTag = generateOpenSearchTotResultsTag ($rescount, $searchResultsDocument);

        $moviematchestag = generate_movies_tags ($results, $searchResultsDocument, $limit);

        $resultsTag->appendChild ($opensearchQueryTag);
        $resultsTag->appendChild ($openSearchTotResultTag);
        $resultsTag->appendChild ($moviematchestag);

        $searchResultsDocument->appendChild ($resultsTag);

        return $searchResultsDocument->saveXML ();
    }

    /* generate an error xml */
    function error_xml ()
    {
        $errorDocument = createDocument ();

        $error = $errorDocument->CreateElement ('error');
        $content = $errorDocument->CreateTextNode (utf8_encode ('You need to search for something first.'));
        $error->appendChild ($content);
        $errorDocument->appendChild ($error);

        return $errorDocument->saveXML ();
    }

    $matches = array ();
    if (preg_match("^title=(.+)$^", $_SERVER['QUERY_STRING'], $matches) == 0)
        echo error_xml ();
    else
    {
        $keywords = $matches[1];
        $search = new imdbsearch();
        $search->setsearchname($keywords);
        $results = $search->results();
        
        if (count($results) > 0)
            echo generate_search_results($keywords, $results, 3);
        else
            echo generate_no_search_results($keywords);
    }
?>
