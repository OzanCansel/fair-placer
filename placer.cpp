#include "placer.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cmath>

namespace placer
{
    struct candidate
    {
        int id;

        friend bool operator ==( const candidate& x , const candidate& y )
        {
            return x.id == y.id;
        }
    };

    struct block
    {
        int         id;
        int         slots;
        std::string desc;

        friend bool operator ==( const block& x , const block& y )
        {
            return x.id == y.id;
        }
    };

    struct place
    {
        int         id;
        int         hardness;
        std::string desc;

        friend bool operator ==( const place& x , const place& y )
        {
            return x.id == y.id;
        }

        friend bool operator <( const place& x , const place& y )
        {
            return x.hardness < y.hardness;
        }

        friend bool operator >( const place& x , const place& y )
        {
            return y < x;
        }
    };

    struct extra_score
    {
        candidate whom;
        int       extra;
    };

    struct placement
    {
        candidate whom;
        place     at;
        block     in;
    };

    struct parse_result
    {
        using accumulator = std::vector<std::pair<candidate , int>>;

        std::vector<candidate> candidates;
        std::vector<block>     blocks;
        std::vector<place>     places;
        accumulator            scores;
    };

    std::vector<std::string> read_lines( std::filesystem::path path , int skip = 1 )
    {
        std::ifstream lines_f { path };

        if ( lines_f.bad() )
            throw std::runtime_error {
                "Error occurred while reading " +
                std::string { path }
            };

        std::vector<std::string> lines;

        while( !lines_f.eof() )
        {
            std::string line;
            std::getline( lines_f , line );

            lines.push_back( line );
        }

        if ( skip == 1 )
        {
            lines.erase(
                std::remove(
                    std::begin( lines ) ,
                    std::end( lines ) ,
                    lines.front()
                )
            );
        }
        else if ( skip > 1 )
        {
            throw std::logic_error { "Not implemented functionality." };
        }

        return lines;
    }

    std::vector<candidate> read_candidates( std::filesystem::path path )
    {
        auto lines = read_lines( path );

        std::vector<candidate> candidates;
        candidates.reserve( std::size( lines ) );

        std::transform(
            std::begin( lines ) ,
            std::end( lines ) ,
            std::back_inserter( candidates ) ,
            []( std::string line ) {
                return candidate { std::atoi( line.c_str() ) };
            }
        );

        return candidates;
    }

    std::vector<block> read_blocks( std::filesystem::path path )
    {
        auto lines = read_lines( path );

        std::vector<block> blocks;
        blocks.reserve( std::size( lines ) );

        std::transform(
            std::begin( lines ) ,
            std::end( lines ) ,
            std::back_inserter( blocks ) ,
            []( std::string line ) {
                std::stringstream ss { line };
                block p;

                ss >> p.id >> p.slots >> p.desc;

                return p;
            }
        );

        return blocks;
    }

    std::vector<place> read_places( std::filesystem::path path )
    {
        auto lines = read_lines( path );

        std::vector<place> places;
        places.reserve( std::size( lines ) );

        std::transform(
            std::begin( lines ) ,
            std::end( lines ) ,
            std::back_inserter( places ) ,
            []( std::string line ) {
                std::stringstream ss { line };
                place p;

                ss >> p.id >> p.hardness >> p.desc;

                return p;
            }
        );

        return places;
    }

    template<typename T , typename U>
    T find_or_throw( const T& searchee , const U& collection )
    {
        auto found = std::find(
            std::begin( collection ) ,
            std::end( collection ) ,
            searchee
        );

        if ( found == std::end( collection ) )
            throw std::runtime_error { "'searchee' could not be found." };

        return *found;
    }

    std::pair<candidate , int>&  find_or_throw(
        const candidate& searchee ,
        std::vector<std::pair<candidate , int>>& accumulator
    )
    {
        auto founded = std::find_if(
            std::begin( accumulator ) ,
            std::end( accumulator ) ,
            [ &searchee ]( const std::pair<candidate , int>& ap )
            {
                return ap.first == searchee;
            }
        );

        if ( founded == std::end( accumulator ) )
            throw std::invalid_argument {
                "'searchee' could not be found."
            };

        return *founded;
    }

    std::vector<placement> read_placement(
        std::filesystem::path path ,
        const std::vector<candidate>& candidates ,
        const std::vector<place>& places ,
        const std::vector<block>& blocks
    )
    {
        if ( path.extension() != ".placement" )
            throw std::invalid_argument { "File extension is not '.placement'." };

        auto lines = read_lines( path );

        std::vector<placement> placements;
        placements.reserve( std::size( lines ) );

        std::transform(
            std::begin( lines ) ,
            std::end( lines ) ,
            std::back_inserter( placements ) ,
            [ & ]( const std::string& line ){
                std::stringstream ss { line };

                placement pl;

                ss >> pl.whom.id >> pl.at.id >> pl.in.id;

                pl.whom = find_or_throw( pl.whom , candidates );
                pl.at   = find_or_throw( pl.at , places );
                pl.in   = find_or_throw( pl.in , blocks );

                return pl;
            }
        );

        return placements;
    }

    std::vector<std::string> list_files( std::filesystem::path dir , std::string extension )
    {
        if ( !std::filesystem::is_directory( dir ) )
            throw std::invalid_argument { "Path is not a directory." };

        std::vector<std::string> placement_files;

        for ( const std::filesystem::directory_entry& e : std::filesystem::directory_iterator { dir } )
        {
            if ( e.is_directory() || e.is_symlink() )
                continue;

            auto f_p = e.path();

            if ( f_p.extension() != extension )
                continue;

            placement_files.push_back(
                std::filesystem::canonical(
                    std::filesystem::absolute( f_p )
                )
            );
        }

        return placement_files;
    }

    std::vector<placement> read_placements(
        std::filesystem::path directory ,
        const std::vector<candidate>& candidates ,
        const std::vector<place>& points ,
        const std::vector<block>& places
    )
    {
        auto placement_files = list_files( directory , ".placement" );

        std::vector<placement> placements;

        for ( const std::string& placement_f : placement_files )
        {
            auto placements_for_f = read_placement( placement_f , candidates , points , places );

            placements.insert(
                std::end( placements ) ,
                std::begin( placements_for_f ) ,
                std::end( placements_for_f )
            );
        }

        return placements;
    }

    std::vector<extra_score> read_extra_score(
        std::filesystem::path score_f_path ,
        const std::vector<candidate>& candidates
    )
    {
        std::vector<extra_score> scores;

        for ( const std::string& line : read_lines( score_f_path ) )
        {
            std::stringstream ss { line };

            extra_score score;

            ss >> score.whom.id;
            ss >> score.extra;

            score.whom = find_or_throw( score.whom , candidates );
            
            scores.push_back( score );
        }

        return scores;
    }

    std::vector<extra_score> read_extra_scores(
        std::filesystem::path directory ,
        const std::vector<candidate>& candidates
    )
    {
        if ( !std::filesystem::is_directory( directory ) )
            throw std::invalid_argument {
                directory.string() + " is not a directory."
            };

        std::vector<extra_score> extras;

        for ( const std::string& f_path : list_files( directory , ".extra" ) )
        {
            auto scores_for_f = read_extra_score( f_path , candidates );

            extras.insert(
                std::end( extras ) ,
                std::begin( scores_for_f ) ,
                std::end( scores_for_f )
            );
        }

        return extras;
    }

    void apply_extra_scores(
        const std::vector<extra_score>& extras ,
        std::vector<std::pair<candidate , int>>& accumulator
    )
    {
        for ( const extra_score& extra : extras )
        {
            auto& [ ap , score ] = find_or_throw( extra.whom , accumulator );

            score += extra.extra;
        }
    }

    void accumulate_scores(
        const std::vector<placement>& placements ,
        std::vector<std::pair<candidate , int>>& accumulator
    )
    {
        for ( const placement& a : placements )
        {
            auto ac = std::find_if(
                std::begin( accumulator ) ,
                std::end( accumulator ) ,
                [ &a ]( const std::pair<candidate , int>& p ){
                    return a.whom == p.first;
                }
            );

            if ( ac == std::end( accumulator ) )
                throw std::runtime_error { "'candidate' doesn't exist." };

            ac->second += a.at.hardness;
        }
    }

    int find_next_idx( std::filesystem::path directory )
    {
        auto placements = list_files( directory , ".placement" );
        int next_idx = 1;

        if ( !placements.empty() )
        {
            std::vector<int> placement_ids;
            placement_ids.reserve( std::size( placements ) );

            std::transform(
                std::begin( placements ) ,
                std::end( placements ) ,
                std::back_inserter( placement_ids ) ,
                []( const std::string& file_path ) {
                    return std::atoi(
                        std::filesystem::path { file_path }.filename().c_str()
                    );
                }
            );

            next_idx = *std::max_element(
                std::begin( placement_ids ) ,
                std::end( placement_ids )
            ) + 1;
        }

        return next_idx;
    }

    std::filesystem::path generate_filename_from_idx( int idx )
    {
        std::stringstream ss;

        ss << std::setfill( '0' ) << std::setw( 6 ) << idx << ".placement";

        return ss.str();
    }

    void write_assignments( std::filesystem::path file , const std::vector<placement>& assignments )
    {
        std::ofstream assignment_f { file };

        if ( assignment_f.bad() )
            throw std::runtime_error {
                "Could not open file" + file.string()
            };
        
        assignment_f << "whom  place  block\n";

        for ( const placement& a : assignments )
        {
            assignment_f << std::left 
                         << std::setw( 7 ) << a.whom.id
                         << std::setw( 7 ) << a.at.id
                         << std::setw( 7 ) << a.in.id
                         << "\n";
        }
    }

    void print_summary( const std::vector<std::pair<candidate , int>>& accumulator )
    {
        std::cout << std::setw( 6 ) << "whom"
                  << std::setw( 6 ) << "score"
                  << "\n";

        for ( const auto& [ a , score ] : accumulator )
        {
            std::cout << std::setw( 6 ) << a.id
                      << std::setw( 6 ) << score
                      << "\n";
        }

        std::cout.flush();
    }

    parse_result parse( std::filesystem::path directory )
    {
        if ( !std::filesystem::is_directory( directory ) )
            throw std::invalid_argument {
                directory.string() + " is not a directory."
            };

        parse_result result;

        result.candidates = read_candidates( directory / "candidates" );
        result.blocks     = read_blocks( directory / "blocks" );
        result.places     = read_places( directory / "places" );

        std::transform(
            std::begin( result.candidates ) ,
            std::end( result.candidates ) ,
            std::back_inserter( result.scores ) ,
            []( const candidate& ap ) {
                return std::make_pair( ap , 0 );
            }
        );

        auto extras = read_extra_scores( directory , result.candidates );

        apply_extra_scores( extras , result.scores );

        auto placements_history = read_placements(
            directory ,
            result.candidates ,
            result.places ,
            result.blocks
        );

        accumulate_scores( placements_history , result.scores );

        return result;
    }

    void next( std::filesystem::path root_p )
    {
        auto result = parse( root_p );

        std::sort(
            std::begin( result.places ) ,
            std::end( result.places ) ,
            std::greater<place> {}
        );

        struct ascending_score
        {
            using entry = std::pair<candidate , int>;

            bool operator()( const entry& x , const entry& y )
            {
                return x.second < y.second;
            }
        };

        std::sort(
            std::begin( result.scores ) ,
            std::end( result.scores ) ,
            ascending_score {}
        );

        std::vector<placement> new_placements;

        new_placements.reserve(
            std::size( result.places ) *
            std::size( result.blocks )
        );

        for ( const place& p : result.places )
        {
            for ( const block& b : result.blocks )
            {
                std::generate_n(
                    std::back_inserter( new_placements ) ,
                    b.slots ,
                    [ & ]{
                        placement new_placement;

                        new_placement.whom = result.scores.front().first;
                        new_placement.at   = p;
                        new_placement.in   = b;

                        std::remove(
                            std::begin( result.scores ) ,
                            std::end( result.scores ) ,
                            result.scores.front()
                        );

                        return new_placement;
                    }
                );
            }
        }

        auto next_idx = find_next_idx( root_p );
        auto filename = generate_filename_from_idx( next_idx );

        write_assignments( root_p / filename , new_placements );
    }

    void summary( std::filesystem::path root )
    {
        auto result = parse( root );

        print_summary( result.scores );
    }

    void print_placement( std::filesystem::path placement_p )
    {
        auto base_dir = std::filesystem::absolute(
            std::filesystem::canonical(
                placement_p
            )
        ).parent_path();

        auto result = parse( base_dir );

        auto placements = read_placement(
            placement_p ,
            result.candidates ,
            result.places ,
            result.blocks
        );

        if ( std::empty( placements ) )
            return;

        auto candidate_col_width = std::max(
            std::size(
                std::to_string(
                    std::max_element(
                        std::begin( result.candidates ) ,
                        std::end( result.candidates ) ,
                        []( const candidate& x , const candidate& y ) {
                            return std::to_string( x.id ).size() <
                                std::to_string( y.id ).size();
                        }
                    )->id
                )
            ) + 2 ,
            std::size_t( 10 )
        );

        auto place_col_width = std::max(
            std::size(
                std::max_element(
                    std::begin( result.places ) ,
                    std::end( result.places ) ,
                    []( const place& x , const place& y ) {
                        return std::size( x.desc ) < std::size( y.desc );
                    }
                )->desc
            ) + 2 ,
            std::size_t( 10 )
        );

        auto block_col_width = std::max(
            std::size(
                std::max_element(
                    std::begin( result.blocks ) ,
                    std::end( result.blocks ) ,
                    []( const block& x , const block& y ) {
                        return std::size( x.desc ) < std::size( y.desc );
                    }
                )->desc
            ) + 2 ,
            std::size_t( 10 )
        );

        auto score_col_width = std::max(
            std::size(
                std::to_string(
                    std::max_element(
                        std::begin( result.places ) ,
                        std::end( result.places ) ,
                        []( const place& x , const place& y ) {
                            return std::to_string( x.hardness ).size() <
                                   std::to_string( y.hardness ).size();
                        }
                    )->hardness
                )
            ) + 2 ,
            std::size_t( 7 )
        );

        std::cout << std::setw( candidate_col_width ) << "candidate"
                  << std::setw( place_col_width )     << "place"
                  << std::setw( block_col_width )     << "block"
                  << std::setw( score_col_width )     << "score"
                  << "\n";

        for ( const placement& pl : placements )
        {
            std::cout << std::setw( candidate_col_width ) << pl.whom.id
                      << std::setw( place_col_width )     << pl.at.desc
                      << std::setw( block_col_width )     << pl.in.desc
                      << std::setw( score_col_width )     << pl.at.hardness
                      << "\n";
        }

        std::cout.flush();
    }
}
