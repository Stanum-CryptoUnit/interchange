#include <eosio/multi_index.hpp>
#include <eosio/contract.hpp>
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/time.hpp>
#include <eosio/print.hpp>
#include <eosio/system.hpp>

class [[eosio::contract]] interchange : public eosio::contract {

public:
    interchange( eosio::name receiver, eosio::name code, eosio::datastream<const char*> ds ): eosio::contract(receiver, code, ds)
    {}
    [[eosio::action]] void approvech(uint64_t id, eosio::name username, uint64_t lock_id, eosio::asset amount);
    [[eosio::action]] void cancel(eosio::name username, uint64_t id, uint64_t est_converted_month);
    [[eosio::action]] void intchange(eosio::name username, eosio::asset quantity, uint64_t lock_id);

    static void create_change_object(eosio::name username, eosio::asset quantity, bool is_lock, uint64_t lock_id);


    static constexpr eosio::name _me = "interchange"_n; 
    static constexpr eosio::name _changer = "changer"_n;   
    static constexpr eosio::name _worldcru = "worldcru"_n;   
    static constexpr eosio::name _tokenlock = "tokenlock"_n;   

    static constexpr eosio::name _token_contract = "eosio.token"_n;
    static constexpr eosio::symbol _cru_symbol     = eosio::symbol(eosio::symbol_code("CRU"), 4);
    static constexpr eosio::symbol _wcru_symbol     = eosio::symbol(eosio::symbol_code("WCRU"), 4);
    
    
    struct [[eosio::table]] changes {
        uint64_t id;
        eosio::time_point_sec date;
        bool is_lock;
        uint64_t lock_id;
        double exchange_rate;
        eosio::name username;
        eosio::asset cru_quantity;
        eosio::asset wcru_quantity;
        eosio::name status;

        uint64_t primary_key() const {return id;}
        uint64_t byusername() const {return username.value;}
        uint64_t bystatus() const {return status.value;}
        uint64_t byvalue() const {return cru_quantity.amount;}

        

        EOSLIB_SERIALIZE(changes, (id)(date)(is_lock)(lock_id)(exchange_rate)(username)(cru_quantity)(wcru_quantity)(status))
    };

    typedef eosio::multi_index<"changes"_n, changes,
    eosio::indexed_by<"byusername"_n, eosio::const_mem_fun<changes, uint64_t, &changes::byusername>>,
    eosio::indexed_by<"bystatus"_n, eosio::const_mem_fun<changes, uint64_t, &changes::bystatus>>,
    eosio::indexed_by<"byvalue"_n, eosio::const_mem_fun<changes, uint64_t, &changes::byvalue>>
    
    > changes_index;


    struct [[eosio::table]] rate {
        uint64_t id;
        double exchange_rate = 1;
        
        uint64_t primary_key() const {return id;}
        
        

        EOSLIB_SERIALIZE(rate, (id)(exchange_rate))
    };

    typedef eosio::multi_index<"rate"_n, rate    > rate_index;



private:
    void apply(uint64_t receiver, uint64_t code, uint64_t action);   
};
