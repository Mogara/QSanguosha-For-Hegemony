--技能讲解6：LuaAPI的扩展
--[[
	大家好，我是数字（Xusine）
	在这一个章节中我将带领大家研究一下国战0.8.3之后版本的Lua接口的深度扩展。
	本章节是建立在你能够独立且正确编写正常Lua的能力上而进行的深度扩展。
	如果你没有较好的Lua基础，可能在阅读本章节的时候会有一些吃力。
	好了，废话不多说，我们开始吧。
]]
--[[
	1.expand_pile
	这个在介绍视为技的时候稍微提了一点，其实就是一个和视为技绑定的Pile。
	在阅读以下内容的时候请先去体验一把邓艾主将急袭的快感……
	在传统的神杀中，这种把Pile上的牌当作某种牌使用都是用技能卡里面askForAG实现的。
	且不说这种方法在写代码时很麻烦，用户体验也是不尽人意。
	然而有了这种方式就能使得技能简洁多了。
	还记得上一章里面的最后一个技能“箭矢”么，如今我们可以把它改成这样一种写法：
]]

--[[
	箭矢：每当你的黑色牌因弃置而失去时，你可将这些牌置于你的武将牌上称为“箭”。回合外你可将两张“箭”当成【无懈可击】使用。
]]

devJianshiVS = sgs.CreateViewAsSkill{ --详细定义可以参考lua\sgs_ex.lua
	name = "devJianshi",
	n = 2,
	expand_pile = "devJianshi", --在这里添加expand_pile成员，在点击按钮的时候就会把Pile移动到手牌中。
	--如果要搞多个Pile的话，可以用逗号隔开。
	view_filter = function(self, selected, to_select)
		if #selected >= 2 or to_select:hasFlag("using") then return false end
		local pat = ".|.|.|devJianshi"
		--expattern 其中最后一个参量位置可以有Pile的名字。
		if string.endsWith(pat, "!") then
			if sgs.Self:isJilei(to_select) then return false end
			pat = string.sub(pat, 1, -2)
		end
		return sgs.Sanguosha:matchExpPattern(pat, sgs.Self, to_select)
	end,
	view_as = function(self,cards) 
		if #cards == 2 then
			local ac = sgs.Sanguosha:cloneCard("nullification")
			ac:setSkillName("devJianshi")
			ac:setShowSkill("devJianshi")
			ac:addSubcard(cards[1])
			ac:addSubcard(cards[2])
			return ac
		end
	end, 
	enabled_at_play = function(self, player)
		return false
	end,
	enabled_at_response = function(self, player, pattern)
		return pattern == "nullification" and player:getPile("devJianshi"):length() > 1
	end,
	enabled_at_nullification = function(self, player)
		return player:getPile("devJianshi"):length() > 1 
	end
}
devJianshi = sgs.CreateTriggerSkill{
	name = "devJianshi",
	view_as_skill = devJianshiVS,
	events = {sgs.BeforeCardsMove},
	can_trigger = function(self, event, room, player, data)
		if player and player:isAlive() and player:hasSkill(self:objectName()) then
			local move = data:toMoveOneTime()
			if move.from and move.from:objectName() ~= player:objectName() then return "" end
			if bit32.band(move.reason.m_reason, sgs.CardMoveReason_S_MASK_BASIC_REASON) == sgs.CardMoveReason_S_REASON_DISCARD then
				for i = 0, move.card_ids:length()-1, 1 do
					local id = move.card_ids:at(i)
					card = sgs.Sanguosha:getCard(id)
					if move.from_places:at(i) == sgs.Player_PlaceHand or move.from_places:at(i) == sgs.Player_PlaceEquip then
						if card:isBlack() then							
							return self:objectName()
						end
					end					
				end
			end
			return ""
		end
	end,
	on_cost = function(self, event, room, player, data)
		if player:askForSkillInvoke(self:objectName(), data) then
			return true 
		end
		return false
	end ,
	on_effect = function(self, event, room, player, data)
		local move = data:toMoveOneTime()
		local card
		local dummy = sgs.Sanguosha:cloneCard("jink") 
		for i = 0, move.card_ids:length()-1, 1 do
			local id = move.card_ids:at(i)
			card = sgs.Sanguosha:getCard(id)
			if move.from_places:at(i) == sgs.Player_PlaceHand or move.from_places:at(i) == sgs.Player_PlaceEquip then
				if card:isBlack() then							
					dummy:addSubcard(id)
				end
			end					
		end
		for _,id in sgs.qlist(dummy:getSubcards()) do
			move.card_ids:removeOne(id)
		end
		data:setValue(move) --如果对data做过更变一定不要忘记setValue
		player:addToPile("devJianshi", dummy:getSubcards())
		dummy:deleteLater() --记住，DummyCard不用一定要删除，否则会造成内存泄漏。
	end,
}
--[[
	通过对上面例子的体会，你是否感受到了expand_pile的神奇作用？
	其实，expand_pile的作用还不光这些，我们再来看一个和它相关的pile:
	2.%pile
	先去完一局有君张角的群雄，试试弘法。
	什么？用别人身上的Pile？看起来不可思议吧。
	是的，在expand_pile里面声明Pile的时候如果有前缀%的话则会从他人身上寻找相关Pile然后移动到手牌。
	我们再来看这样一个技能。
]]

--[[
	施仁:	君主技，回合开始时，若你的武将牌上没有牌，你可以翻开牌堆顶X张牌，将其中的基本牌和【无懈可击】放置于武将牌上，将其他牌置于弃牌堆.(X为当前与你势力相同的角色*3)
	君主技，只要你的武将牌上有牌，你拥有“开发组”
	#开发组：
	任意一名与你势力相同的角色需要使用或者打出牌时，若你的武将牌上有需要的牌，可以从你的武将牌上使用或打出之。
]]

--[[
	分析：
	这个技能可以说是一个比较复杂的技能了，结合上面的经验，我们应该考虑使用expand_pile。
	为了增强用户体验，我们还需要控制好enable_at_*****函数。
	代码如下：
]]

devShiren = sgs.CreateTriggerSkill{
	name = "devShiren$" ,
	events = {sgs.EventPhaseStart},
	can_trigger = function(self, event, room, player, data)
		if player and player:isAlive() and player:hasSkill(self:objectName()) then
			if player:getPhase() == sgs.Player_Start and player:getPile("devShiren"):length() < player:getPlayerNumWithSameKingdom(self:objectName()) then
				return self:objectName()
			else
				return ""
			end
		end
		return ""
	end ,
	on_cost = function(self, event, room, player, data)
		if player:hasShownSkill(self) or player:askForSkillInvoke(self:objectName(), data) then
			room:notifySkillInvoked(player, self:objectName())
			room:broadcastSkillInvoke(self:objectName())
			return true 
		end
		return false
	end ,
	on_effect = function(self, event, room, player, data)
		local shiren = room:getNCards(player:getPlayerNumWithSameKingdom(self:objectName())*3)
		for _,id in sgs.qlist(shiren)do
			local c = sgs.Sanguosha:getCard(id)
			if c:isKindOf("BasicCard") or c:isKindOf("Nullification") then
				player:addToPile("devShiren",c,true)
			else
				room:throwCard(id,player,nil,self:objectName())
			end
		end
		return false
	end ,
}
--[[
	上面的技能是为了向Pile里面装牌。
]]

devShirenAddSkill = sgs.CreateTriggerSkill{ --这个技能是为了添加技能
	name = "devShirenAddSkill" ,
	global = true,
	events = {sgs.GeneralShown,sgs.EventLoseSkill},
	can_trigger = function(self, event, room, player, data)
		if player and player:isAlive() then
			if event == sgs.GeneralShown then
				local source = room:findPlayerBySkillName("devShiren")
				if source and source:isAlive() and source:hasShownSkill("devShiren") then
					for _,p in sgs.qlist(room:getAlivePlayers())do
						if p:isFriendWith(source) and not p:hasSkill("devShirenAsk") then
							room:attachSkillToPlayer(p,"devShirenAsk")
							source:speak(p:screenName())
						end
					end
				end
			else
				local skill_name = data:toString()
				if skill_name == "devShiren" then
					for _,p in sgs.qlist(room:getAlivePlayers())do
						if p:hasSkill("devShirenAsk") then
							room:detachSkillFromPlayer(p,"devShirenAsk")
						end
					end
				end
			end
		end
		return ""
	end ,
}
addSkillToEngine(devShirenAddSkill) --上文定义了这个函数，作用是为了把技能加入Engine

devShirenAsk = sgs.CreateOneCardViewAsSkill{ --视为技是整个技能的核心。
	name = "devShirenAsk",
	expand_pile = "devShiren,%devShiren",
	--不可缺少的expand_pile,用逗号隔开可以写多个
	--其中第一个devShiren是给本人用的
	--第二个%devShiren则是给其他同势力角色用的。
	view_filter = function(self,to_select)
		local pat = ".|.|.|devShiren,%devShiren" --和expand_pile相照应的Pattern
		if not sgs.Sanguosha:matchExpPattern(pat, sgs.Self, to_select) then return false end
		if sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE or sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE then --非主动使用的情况
			local pattern = sgs.Sanguosha:getCurrentCardUsePattern()
			if to_select:match(pattern) or (pattern == "nullification" and to_select:isKindOf("Nullification")) then
				return true
			else
				return false
			end
		else --主动使用的情况
			if to_select:isAvailable(sgs.Self) then
				return true
			else
				return false
			end
		end
		return false
	end,
	view_as = function(self,origin) --直接返回原卡，不做改变
		return origin
	end,
	enabled_at_play = function(self, player) --检测能不能主动使用
		local bool = false
		local lord = player:getLord()
		if lord == nil then return false end
		local pile = lord:getPile("devShiren")
		for _,id in sgs.qlist(pile)do
			local c = sgs.Sanguosha:getCard(id)
			if c:isAvailable(player) then
				bool = true
				break
			end
		end
		return bool
	end,
	enabled_at_response = function(self, player, pattern) --主动打出
		sgs.ShirenPattern = pattern
		local bool = false
		local lord = player:getLord()
		local pile = lord:getPile("devShiren")
		for _,id in sgs.qlist(pile)do
			local c = sgs.Sanguosha:getCard(id)
			if pattern == "nullification" then
				if c:isKindOf("Nullification") then
					bool = true
					break
				end
			end
			if c:match(pattern) then --切记这里的match不是Lua的match函数，是Card::match
				bool = true
				break
			end
		end
		return bool
	end,
	enabled_at_nullification = function(self,player) --相应无懈可击
		local lord = player:getLord()
		if lord then
			local pile = lord:getPile("devShiren")
			for _,id in sgs.qlist(pile)do
				local c = sgs.Sanguosha:getCard(id)
				if c:isKindOf("Nullification") then
					return true
				end
			end
		end
		return false
	end,
}
--[[
	如果上面这个技能你研究明白了的话，基本上你就可以加入我们开发组了^_^
	（如果你有能力而又想加入开发组的可以联系数字。 Email : xusine@mogara.org ）
	当然，研究明白也不要紧；自己改着一些东西试试就好了。
	好的，我们再来看另外一个Pile：
	3.&Pile
	Pile名以&打头的都会想木牛流马那样，在使用和打出时会被视为手牌。
	先来看这样一个技能：
]]

--[[
	诊断：每当一名角色开始判定时，你可以把牌堆顶两张牌放到武将牌上，称之为“诊断牌”。你可以将诊断牌当作手牌使用或打出。
]]

--不废话了，直接上技能：

devZhenduan = sgs.CreateTriggerSkill{
	name = "devZhenduan",
	events = {sgs.StartJudge},
	can_trigger = function(self, event, room, player, data)
		if player and player:isAlive() then
			local source = room:findPlayerBySkillName(self:objectName())
			if source and source:isAlive() then
				return self:objectName(),source
			end
		end
	end,
	on_cost = function(self, event, room, player, data,ask_who)
		return room:askForSkillInvoke(ask_who,self:objectName(),data)
	end,
	on_effect = function(self, evnet, room, player, data,ask_who)
		ask_who:addToPile("&devZhenduan",room:getNCards(2)) --配合甄姬会非常喜感……
		--[[
			请注意这里的&devZhenduan这个Pile，在翻译和操作的时候&必须保留。
		]]
		return false
	end,
}

--[[
	其实合理使用上面三个Pile可以完成不少看似困难的工作。
	值得注意的是，对于Lua视为技，&Pile需要手动在视为技中搞定。
	我们再来看另外一个在客户端能够使用的东西。
	4.ServerInfo
	顾名思义，就是服务器信息。它的定义如下：
]]
struct ServerInfoStruct {
    const QString Name; --服务器名字
    const QString GameMode; --游戏模式，具体地，请参考源代码
    const int OperationTimeout; --操作时间
    const int NullificationCountDown; --无懈可击时间
    const QStringList Extensions; --扩展包
    const bool RandomSeat; --随机座位
    const bool EnableCheat; --允许作弊
    const bool FreeChoose; --自由选将
    const bool ForbidAddingRobot; --禁止AI
    const bool DisableChat; --禁止聊天
    const bool FirstShowingReward; --首亮摸牌

    const bool DuringGame; --在游戏中
};

extern ServerInfoStruct ServerInfo;

--这是C++代码，在Lua文件中显示不到好处……
--当然，你也可以参考swig/sanguosha.i 文件。
--[[
	从定义可以，我们可以通过sgs.ServerInfo来访问这个结构体，比如说
	sgs.ServerInfo.Name神马的
	由于这个比较简单，所以这里就不详细介绍了。
	5.SkipGameRule
	从字面上看就是跳过游戏规则，我们来看这样一个技能。
]]

--[[
	猜疑：主将技，每当你使用【杀】指定一名角色时，可以让目标角色使用【杀】来响应。
]]
--[[
	分析：
	这个技能改变了正常的游戏进程，因此务必需要用SkipGameRule来完成。
	代码如下：
]]
devCaiyi = sgs.CreateTriggerSkill{
	name = "devCaiyi",
	events = {sgs.SlashProceed},
	relate_to_place = "head",
	can_trigger = function(self, event, room, player, data)
		local effect = data:toSlashEffect()
		if effect.from and effect.from:isAlive() and effect.from:hasSkill(self:objectName()) then
			return self:objectName()
		end
	end ,
	on_cost = function(self, event, room, player, data)
		local ai_data = sgs.QVariant()
		ai_data:setValue(data:toSlashEffect().to)
		if room:askForSkillInvoke(player,self:objectName(),ai_data) then
			return true
		end
	end ,
	on_effect = function(self, evnet, room, player, data) --这些内容来自GameRule.cpp
		local effect = data:toSlashEffect()
		if effect.jink_num == 1 then
            local jink = room:askForCard(effect.to, "slash", "slash-slash:" .. effect.from:objectName(), data, sgs.Card_MethodResponse, effect.from)
			----可能这里的handling_method有一些问题，我会把它换成打出。
			if room:isJinkEffected(effect.to, jink) then else jink = nil end
            room:slashResult(effect, jink)
        else
            local jink = sgs.Sanguosha:cloneCard("slash")
            local asked_jink = nil
            for i = 1,effect.jink_num,1 do
				local prompt
				if i == 1 then
					prompt = ("@multi-slash%s:%s::%d"):format("-start",effect.from:objectName(),i)
				else
					prompt = ("@multi-slash%s:%s::%d"):format("",effect.from:objectName(),i)
				end
                asked_jink = room:askForCard(effect.to, "slash", prompt, data, sgs.Card_MethodResponse, effect.from)
				--同上。
                if ( not room:isJinkEffected(effect.to, asked_jink)) then
					jink:deleteLater()
                    jink = nil
					break
                else
                    jink:addSubcard(asked_jink:getEffectiveId())
                end
            end
            room:slashResult(effect, jink)
        end
		room:setTag("SkipGameRule",sgs.QVariant(true)) --这里SkipGameRule了。一般是最后再SkipGameRule的。
	end ,
	priority = 1 --如果SkipGameRule的话，优先权一定为1
}
--[[
	好像也并不是很难啊。
	6.extra_cost 
	这个是一个技能卡属性，一般在拼点的时候会使用到。
	来看这样一个技能：
]]

--[[
	热心：	副将技，回合外每当一名角色进入濒死阶段时，你可以与当前角色拼点，若你赢，视为你对当前濒死角色使用一张【桃】。你可以重复此过程直到你拼点失败或者不想继续拼点为止。
]]

devRexinCard=sgs.CreateSkillCard{
	name="devRexinCard",
	will_throw = false,
	skill_name = "devRexin",
	filter = function(self, targets, to_select)
		return #targets == 0 and to_select:getPhase() ~= sgs.Player_NotActive and to_select:objectName() ~= sgs.Self:objectName()
	end ,
	extra_cost = function(self,room,card_use) --要在这个时候询问拼点牌和记录拼点过程
		local source = card_use.from
		local subcard = sgs.Sanguosha:getCard(self:getEffectiveId())
		local pd 
		if subcard then
			pd = source:pindianSelect(room:getCurrent(),"devRexin",subcard)
		else
			pd = source:pindianSelect(room:getCurrent(),"devRexin")
		end
		local data = sgs.QVariant()
		data:setValue(pd)
		source:setTag("devRexin",data)
	end,
	on_effect = function(self,effect)
		local pd = effect.from:getTag("devRexin"):toPindian()
		effect.from:removeTag("devRexin")
		local success = effect.from:pindian(pd) --在这里拼点产生结果。
		if success then
			local peach = sgs.Sanguosha:cloneCard("peach")
			peach:setSkillName("devRexin")
			peach:deleteLater()
			effect.from:getRoom():useCard(sgs.CardUseStruct(peach,effect.from,effect.from:getRoom():getCurrentDyingPlayer()))
		else
			effect.from:getRoom():setPlayerFlag(effect.from,"Global_devRexinFailed")
			--[[
				在这里说一下Global_***Failed这个Flag
				这个Flag会在恰当的时机（ChoiceMade）被系统清除。
				所以，感觉应该被限制的时候就用吧……
				对于ChoiceMade，可以参考神杀Lua吧里面的一篇ChoiceMade的说明
			]]
		end
	end
}
devRexin = sgs.CreateOneCardViewAsSkill{
	name = "devRexin",
	enabled_at_play = function() --一定不能主动使用
		return false
	end,
	filter_pattern = ".",
	enabled_at_response=function(self, player, pattern)
		if player:hasFlag("Global_devRexinFailed") or player:hasFlag("Global_PreventPeach") then return false end --处理完杀和失败。
		if player:isKongcheng() then return false end
		if not string.find(pattern,"peach") then return false end
		for _,p in sgs.qlist(player:getAliveSiblings()) do --迭代当前客户端的所有玩家
			if p:getPhase() ~= sgs.Player_NotActive and not p:isKongcheng() then --当前回合角色不空城
				return true
			end
		end
		return false
		end,
	view_as = function(self,card)
		local acard = devRexinCard:clone()
		acard:addSubcard(card)
		acard:setShowSkill(self:objectName()) --别忘了
		acard:setSkillName(self:objectName())
		return acard
	end,
}
--[[
	其实这是神杀为了规则而设计的一个接口，在extra_cost的时候玩家还没有明置武将，所以可以执行一些效果。
	当然，扩展性的内容不仅仅就这么多，有一些已经介绍过了，具体地，可以参考下面的文档：
	1.视为技部分
	2.使用Json来控制客户端
	3.触发技部分
]]